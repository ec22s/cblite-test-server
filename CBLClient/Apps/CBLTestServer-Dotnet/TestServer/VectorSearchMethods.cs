using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Net;

using JetBrains.Annotations;

using Couchbase.Lite;
using Couchbase.Lite.Enterprise.Query;
using Couchbase.Lite.Logging;
using Couchbase.Lite.Query;

using static Couchbase.Lite.MutableDictionaryObject;
using static Couchbase.Lite.DatabaseConfiguration;
using static Couchbase.Lite.Database;

using static Couchbase.Lite.Testing.DatabaseMethods;

namespace Couchbase.Lite.Testing
{
    public static class VectorSearchMethods
    {
        public static MutableDictionaryObject wordMap;
        private static readonly bool UseInMemoryDb = true;
        private static readonly string InMemoryDbName = "vsTestDatabase";


        public static void CreateIndex([NotNull] NameValueCollection args,
                                       [NotNull] IReadOnlyDictionary<string, object> postBody,
                                       [NotNull] HttpListenerResponse response)
        {
            // using db get defualt collection and store in Collection collection
            With<Database>(postBody, "database", database =>
            {
                Collection collection = database.GetCollection(postBody["collectionName"].ToString(), "_default"); // TO: change the hardocded scope
                // get values from postBody
                string indexName = postBody["indexName"].ToString();
                string expression = postBody["expression"].ToString();

                // null coalescing checks
                uint dimensions = Convert.ToUInt32(postBody["dimensions"]);

                uint centroids = Convert.ToUInt32(postBody["centroids"]);

                uint minTrainingSize = Convert.ToUInt32(postBody["minTrainingSize"]);

                uint maxTrainingSize = Convert.ToUInt32(postBody["maxTrainingSize"]);

                uint? bits = 0;
                uint? subquantizers = 0;
                ScalarQuantizerType? scalarEncoding = new();

                try
                {
                    bits = Convert.ToUInt32(postBody["bits"]);
                    subquantizers = Convert.ToUInt32(postBody["subquantizers"]);
                }
                catch (Exception e)
                {
                    bits = null;
                    subquantizers = null;
                }

                try
                {
                    scalarEncoding = (ScalarQuantizerType)postBody["scalarEncoding"];
                }
                catch (Exception e)
                {
                    scalarEncoding = null;
                }




                string metric = postBody["metric"].ToString();

                // correctness checks
                if (scalarEncoding != null && (bits != null || subquantizers != null))
                {
                    throw new InvalidOperationException("Cannot have scalar quantization and arguments for product quantization at the same time");
                }

                if ((bits != null && subquantizers == null) || (bits == null && subquantizers != null))
                {
                    throw new InvalidOperationException("Product quantization requires both bits and subquantizers set");
                }


                // setting values based on config from input
                VectorEncoding encoding = VectorEncoding.None();
                if (scalarEncoding != null)
                {
                    encoding = VectorEncoding.ScalarQuantizer((ScalarQuantizerType)scalarEncoding);
                }
                if (bits != null)
                {
                    encoding = VectorEncoding.ProductQuantizer((uint)subquantizers, (uint)bits);
                }
                DistanceMetric dMetric = new();
                if (metric != null)
                {
                    dMetric = metric switch
                    {
                        "euclidean" => DistanceMetric.Euclidean,
                        "cosine" => DistanceMetric.Cosine,
                        _ => throw new Exception("Invalid distance metric"),
                    };
                }

                VectorIndexConfiguration config = new(expression, dimensions, centroids) // unure on types here again, undocumented specifics
                {
                    Encoding = encoding,
                    DistanceMetric = dMetric,
                    MinTrainingSize = minTrainingSize,
                    MaxTrainingSize = maxTrainingSize
                };

                try
                {
                    collection.CreateIndex(indexName, config);
                    Console.WriteLine("Successfully created index");
                    response.WriteBody("Created index with name " + indexName);
                }
                catch (Exception e)
                {
                    Console.WriteLine("Failed to create index");
                    response.WriteBody("Could not create index: " + e);
                }

            });

        }

        public static void RegisterModel([NotNull] NameValueCollection args,
                                  [NotNull] IReadOnlyDictionary<string, object> postBody,
                                  [NotNull] HttpListenerResponse response)
        {
            string modelName = postBody["name"].ToString();
            string key = postBody["key"].ToString();

            VectorModel vectorModel = new(key);
            Database.Prediction.RegisterModel(modelName, vectorModel);
            Console.WriteLine("Successfully registered Model");
            response.WriteBody("Successfully registered model: " + modelName);
        }

        static MutableDictionaryObject GetWordVectMap()
        {
            Database db = new(InMemoryDbName);
            try
            {
                string sql1 = string.Format("select word, vector from auxiliaryWords");
                IQuery query1 = db.CreateQuery(sql1);
                IResultSet rs1 = query1.Execute();
                string sql2 = string.Format("select word, vector from searchTerms");
                IQuery query2 = db.CreateQuery(sql2);
                IResultSet rs2 = query2.Execute();

                MutableDictionaryObject words = new();
                List<Result> rl = rs1.AllResults();
                List<Result> rl2 = rs2.AllResults();
                rl.AddRange(rl2);

                foreach (Result r in rl)
                {
                    string word = r.GetString("word");
                    ArrayObject vector = r.GetArray("vector");
                    words.SetValue(word, vector);
                }
                db.Close();
                return words;

            }
            catch (Exception e)
            {
                Console.WriteLine(e + "retrieving vector could not be done - getWordVector query returned no results");
                db.Close();
                return null;
            }
        }

        public static void LoadDatabase([NotNull] NameValueCollection args,
                                  [NotNull] IReadOnlyDictionary<string, object> postBody,
                                  [NotNull] HttpListenerResponse response)
        {
            if (UseInMemoryDb)
            {
                string dbID = PreparePredefinedDatabase(InMemoryDbName);
                wordMap = GetWordVectMap();
                response.WriteBody(dbID);
            }
            else
            {
                string dbID = PreparePredefinedDatabase("dummtDBIgnoreIt");
                response.WriteBody(dbID);
            }

        }

        public static void GetEmbedding([NotNull] NameValueCollection args,
                                  [NotNull] IReadOnlyDictionary<string, object> postBody,
                                  [NotNull] HttpListenerResponse response)
        {
            DictionaryObject value = GetEmbeddingDic(postBody["input"].ToString());
            Dictionary<String, Object> vectorDict = value.ToDictionary();
            List<object> embedding = (List<object>)vectorDict["vector"];
            response.WriteBody(embedding);
        }

        public static void Query([NotNull] NameValueCollection args,
                                  [NotNull] IReadOnlyDictionary<string, object> postBody,
                                  [NotNull] HttpListenerResponse response)
        {
            object term = postBody["term"];

            With<Database>(postBody, "database", db =>
            {
                DictionaryObject embeddedTermDic = GetEmbeddingDic(term.ToString());
                var embeddedTerm = embeddedTermDic.GetValue("vector");
                string sql = postBody["sql"].ToString();
                Console.WriteLine("QE-DEBUG Calling query string: " + sql);

                IQuery query = db.CreateQuery(sql);
                query.Parameters.SetValue("vector", embeddedTerm);

                List<object> resultArray = new();
                int c = 0;
                foreach (Result row in query.Execute())
                {
                    resultArray.Add(row.ToDictionary());
                    c++;
                }
                response.WriteBody(resultArray);
            });
        }

        private static DictionaryObject GetEmbeddingDic(string term)
        {
            VectorModel model = new("word");
            MutableDictionaryObject testDic = new();
            testDic.SetValue("word", term);
            DictionaryObject value = model.Predict(testDic);
            return value;
        }

        private static string PreparePredefinedDatabase(string dbName)
        {
            string dbPath = "Databases\\vsTestDatabase.cblite2\\";
            string strExeFilePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
            string strWorkPath = System.IO.Path.GetDirectoryName(strExeFilePath);
            string databasePath = Path.Combine(strWorkPath, dbPath);
            DatabaseConfiguration dbConfig = new();
            Database.Copy(databasePath, dbName, dbConfig);
            var db = MemoryMap.New<Database>(dbName, dbConfig);
            Console.WriteLine("Succesfully loaded database " + dbName);
            return db;
        }


        public sealed class VectorModel : IPredictiveModel
        {
            public string key;

            public VectorModel(string key)
            {
                this.key = key;
                Console.WriteLine("QE-DEBUG Vector Model object instantiated with key: " + key);
            }

            public DictionaryObject? Predict(DictionaryObject input)
            {
                Console.WriteLine("QE-DEBUG Calling predict function");
                string inputWord = input.GetString(key);
                Console.WriteLine("QE-DEBUG Predicting for word: " + inputWord);
                object result = new();
                result = wordMap.GetValue(inputWord);
                if (result == null) {
                    Console.WriteLine("QE-DEBUG Prediction gave null result");
                } else {
                    Console.WriteLine("QE-DEBUG Prediction gave non-null result");
                }
                MutableDictionaryObject output = new();
                output.SetValue("vector", result);
                return output;
            }
        }

    }
}
