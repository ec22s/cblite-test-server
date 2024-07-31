package com.couchbase.mobiletestkit.javacommon.RequestHandler;

import java.util.List;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import com.couchbase.mobiletestkit.javacommon.*;
import com.couchbase.lite.*;
import com.couchbase.lite.internal.utils.FileUtils;
import com.couchbase.mobiletestkit.javacommon.util.Log;

public class VectorSearchRequestHandler {
    private static  Map<String, Array> wordMap;
    private static final Boolean useInMemoryDb = true;
    private static final String inMemoryDbName = "InMemoryDb";
    
    static Map<String, Array> getWordVectMap() {
        try {
            DatabaseRequestHandler dbHandler = new DatabaseRequestHandler();
            Database db = preparePredefinedDatabase(inMemoryDbName);

            String sql1 = String.format("select word, vector from auxiliaryWords");
            Query query1 = db.createQuery(sql1);
            ResultSet rs1 = query1.execute();
            String sql2 = String.format("select word, vector from searchTerms");
            Query query2 = db.createQuery(sql2);
            ResultSet rs2 = query2.execute();

            Map<String, Array> words = new HashMap<String, Array>();
            List<Result> rl = rs1.allResults();
            List<Result> rl2 = rs2.allResults();
            rl.addAll(rl2);

            for (Result r : rl) {
                String word = r.getString("word");
                Array vector = r.getArray("vector");
                words.put(word, vector);
            }
            return words;

        } catch (Exception e) {
            System.err.println(e + "retrieving vector could not be done - getWordVector query returned no results");
            return null;
        }
    }

    public String createIndex(Args args) throws CouchbaseLiteException, Exception {
        Database database = args.get("database");
        String scopeName = args.get("scopeName") != null ? args.get("scopeName") : "_default";
        String collectionName = args.get("collectionName") != null ? args.get("collectionName") : "_default";

        Collection collection = database.getCollection(collectionName, scopeName);
        if (collection == null) {
            throw new Exception("Could not open specified collection");
        }

        String indexName = args.get("indexName");

        String expression = args.get("expression");

        int dimensions = args.get("dimensions");
        int centroids = args.get("centroids");
        Boolean isLazy = args.get("isLazy");

        VectorEncoding.ScalarQuantizerType scalarEncoding = args.get("scalarEncoding");

        Integer subquantizers = args.get("subquantizers");
        Integer bits = args.get("bits");

        String metric = args.get("metric");

        Integer minTrainingSize = args.get("minTrainingSize");
        Integer maxTrainingSize = args.get("maxTrainingSize");

        if (scalarEncoding != null && (bits != null || subquantizers != null)) {
            throw new Exception(
                    "Cannot have scalar quantization and arguments for product quantization at the same time");
        }

        if ((bits != null && subquantizers == null) || (bits == null && subquantizers != null)) {
            throw new Exception("Product quantization requires both bits and subquantizers set");
        }

        VectorIndexConfiguration config = new VectorIndexConfiguration(expression, dimensions, centroids);
        if (scalarEncoding != null) {
            config.setEncoding(VectorEncoding.scalarQuantizer(scalarEncoding));
        }
        if (bits != null) {
            config.setEncoding(VectorEncoding.productQuantizer(subquantizers, bits));
        }
        if (metric != null) {
            switch (metric) {
                case "euclidean":
                    config.setMetric(VectorIndexConfiguration.DistanceMetric.EUCLIDEAN);
                    break;
                case "cosine":
                    config.setMetric(VectorIndexConfiguration.DistanceMetric.COSINE);
                    break;
                default:
                    throw new Error("Invalid distance metric");
            }
        }

        if (minTrainingSize != null) {
            config.setMinTrainingSize(minTrainingSize);
        }

        if (maxTrainingSize != null) {
            config.setMaxTrainingSize(maxTrainingSize);
        }
    /*    if (isLazy != null) {
            config.isLazy = isLazy;
        }*/
        try {
            collection.createIndex(indexName, config);
            return String.format("Created index with name %s on collection %s", indexName, collectionName);
        } catch (Exception e) {
            return "Could not create index: " + e;
        }
    }

   /*  public String updateIndex(Args args) {
        String collection = args.get("collection");
        String indexName = args.get("indexName");
        QueryIndex config = new QueryIndex(collection, indexName);
        return "Temp dummy return";
    }*/

    public String registerModel(Args args) {
        String key = args.get("key");
        String name = args.get("name");
        vectorModel model = new vectorModel(key, inMemoryDbName);
        Database.prediction.registerModel(name, model);
        return "Registered model with name " + name;
    }

    public List<Object> query(Args args) throws CouchbaseLiteException, IOException {
        String term = args.get("term");

        Args embeddingArgs = new Args();
        embeddingArgs.put("input", term);
        Object embeddedTerm = this.getEmbedding(embeddingArgs);

        String sql = args.get("sql");

        Database db = args.get("database");

        Parameters params = new Parameters();
        params.setValue("vector", embeddedTerm);
        Query query = db.createQuery(sql);
        query.setParameters(params);

        List<Object> resultArray = new ArrayList<>();
        try (ResultSet rs = query.execute()) {
            for (Result row : rs) {
                resultArray.add(row.toMap());
            }
        }

        return resultArray;
    }

    public Object getEmbedding(Args args) throws CouchbaseLiteException, IOException {
        vectorModel model1 = new vectorModel("word", inMemoryDbName);
        MutableDictionary testDic = new MutableDictionary();
        String input = args.get("input");
        testDic.setValue("word", input);
        Dictionary value = model1.predict(testDic);
        return value.getValue("vector");
    }

    public Database loadDatabase(Args args) throws CouchbaseLiteException, IOException {
        if (useInMemoryDb) {
            wordMap = getWordVectMap();
            Database db = new Database(inMemoryDbName);
            return db;
        }
        else {
            Database db = preparePredefinedDatabase("dummtDBIgnoreIt");
            return db;
        }
    }

    private class vectorModel implements PredictiveModel {
        String key;
        String dbName;

        vectorModel(String key, String name) {
            try {
                this.key = key;
                this.dbName = name;
            } catch (Exception e) {
                System.err.println(e + "Error creating instance of vectorModel");
            }
        }

        @Override
        public Dictionary predict(Dictionary input) {
            String inputWord = input.getString(this.key);
            Object result = new ArrayList<>();
            result = wordMap.get(inputWord);
            MutableDictionary output = new MutableDictionary();
            output.setValue("vector", result);
            return output;
        }

    }

    private static Database preparePredefinedDatabase(String dbName) throws CouchbaseLiteException, IOException {
         // loads the given database vsTestDatabase
         DatabaseRequestHandler dbHandler = new DatabaseRequestHandler();
         Args newArgs = new Args();
         newArgs.put("dbPath", "vstestDatabase.cblite2");
         String dbPath = dbHandler.getVectorSearchDb(newArgs);
         newArgs.put("directory", "");
         if (RequestHandlerDispatcher.context.getPlatform().equals("android")) {
             newArgs.put("directory", new File(dbPath).getParent());
         }
         Database.exists("vstestDatabase.cblite2", new File(dbPath));
         DatabaseConfigurationRequestHandler configHandler = new DatabaseConfigurationRequestHandler();
         DatabaseConfiguration dbConfig = new DatabaseConfiguration();
         dbConfig = configHandler.configure(newArgs);
         newArgs.put("dbPath", dbPath);
         newArgs.put("dbName", dbName);
         newArgs.put("dbConfig", dbConfig);
         dbHandler.copy(newArgs);
         Database db1 = new Database(dbName);

         return db1;
    }
}