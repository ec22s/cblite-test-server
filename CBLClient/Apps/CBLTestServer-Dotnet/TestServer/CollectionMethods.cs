using System;
using System.IO;
using System.Reflection;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Net;
using System.Linq;
using System.Security.Cryptography.X509Certificates;
using System.Runtime.Serialization.Formatters.Binary;

using Couchbase.Lite.Sync;
using Couchbase.Lite.Util;

using JetBrains.Annotations;

using Newtonsoft.Json.Linq;

using static Couchbase.Lite.Testing.DatabaseMethods;
using System.Threading;
using Couchbase.Lite.Query;

namespace Couchbase.Lite.Testing
{
    public class CollectionMethods
    {
        public static void defaultCollection([NotNull] NameValueCollection args,
                                             [NotNull] IReadOnlyDictionary<string, object> postBody,
                                             [NotNull] HttpListenerResponse response)
        {
            With<Database>(postBody, "database", database =>
            {
                response.WriteBody(MemoryMap.Store(database.GetDefaultCollection()));
            });
        }

        public static void createCollection([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Database>(postBody, "database", database =>
            {
                String collectionName = postBody["collectionName"].ToString();
                String scopeName;
                if (postBody.ContainsKey("scopeName"))
                    scopeName = postBody["scopeName"].ToString();
                else
                    scopeName = "_default";
                response.WriteBody(MemoryMap.Store(database.CreateCollection(collectionName, scopeName)));
            });
        }

        public static void deleteCollection([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Database>(postBody, "database", database =>
            {
                string collectionName = postBody["collectionName"].ToString();
                string scopeName;
                if (postBody.ContainsKey("scopeName"))
                    scopeName = postBody["scopeName"].ToString();
                else
                    scopeName = "_default";
                database.DeleteCollection(collectionName, scopeName);
                response.WriteBody(true);
            });
            response.WriteEmptyBody();
        }

        public static void collectionNames([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            string scopeName = "_default";
            if(postBody.ContainsKey("scopeName"))
            {
                scopeName = postBody["scopeName"].ToString();
            }
            With<Database>(postBody, "database", database =>
            {
                
                List<string> collectionNames = new List<string>();
                IReadOnlyList<Collection> collectionObjects = database.GetCollections(scopeName);
                foreach (Collection col in collectionObjects)
                {
                    string name = col.Name;
                    collectionNames.Add(name);
                }
                response.WriteBody(collectionNames);
            });
        }

        public static void collectionName([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                response.WriteBody(collection.Name);
            });
        }

        public static void collectionObject([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Database>(postBody, "database", database =>
            {
                string collectionName = postBody["collectionName"].ToString();
                string scopeName = postBody["scopeName"].ToString();
                response.WriteBody(MemoryMap.Store(database.GetCollection(collectionName, scopeName)));
            });
        }

        public static void getDocIds([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                    IExpression limit = Expression.Int((int)postBody["limit"]);
                    IExpression offset = Expression.Int((int)postBody["offset"]);
                    using (IQuery query = Query.QueryBuilder
                           .Select(SelectResult.Expression(Meta.ID))
                           .From(DataSource.Collection(collection))
                           .Limit(limit, offset))
                    {
                        var result = query.Execute();
                        var ids = result.Select(x => x.GetString("id")).ToList();
                        response.WriteBody(ids);
                    }
                });
        }

        public static void documentCount([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                response.WriteBody(collection.Count);
            });
        }

        public static void saveDocument([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                With<MutableDocument>(postBody, "document", document =>
                {
                    collection.Save(document);
                });
            });
            response.WriteEmptyBody();
        }


        public static void collectionScope([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                response.WriteBody(MemoryMap.Store(collection.Scope));
            });
        }
        public static void getDocument([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                String docId = postBody["docId"].ToString();
                response.WriteBody(MemoryMap.Store(collection.GetDocument(docId)));
            });
        }

        public static void getDocuments([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            var ids = ((List<object>)postBody["ids"]).OfType<string>();
            With<Collection>(postBody, "collection", collection =>
            {
                var documents = new Dictionary<string, object>();
                using (var query = Query.QueryBuilder
                       .Select(SelectResult.Expression(Meta.ID))
                    .From(DataSource.Collection(collection)))
                {
                    var result = query.Execute();
                    foreach (var id in ids)
                    {
                        Document document = collection.GetDocument(id);
                        if (document != null)
                        {
                            var doc = document.ToDictionary();

                            foreach (KeyValuePair<String, object> item in document)
                            {
                                Dictionary<string, object> updatedValue = new Dictionary<string, object>();
                                bool isBlob = false;
                                if (item.Value != null && item.Value.GetType() == typeof(Couchbase.Lite.DictionaryObject))
                                {
                                    foreach (KeyValuePair<String, object> innerVal in (Couchbase.Lite.DictionaryObject)item.Value)
                                    {
                                        if (innerVal.Value != null && innerVal.Value is Blob)
                                        {
                                            isBlob = true;
                                            Blob blob = (Blob)innerVal.Value;
                                            Dictionary<string, object> properties = new Dictionary<string, object>();
                                            foreach (KeyValuePair<string, object> p in blob.Properties)
                                            {
                                                properties.Add(p.Key, p.Value);
                                            }
                                            updatedValue.Add(innerVal.Key, properties);
                                        }
                                    }
                                    if (isBlob && updatedValue != null)
                                    {
                                        doc[item.Key] = updatedValue;
                                    }
                                }
                            }
                            documents[id] = doc;
                        }
                    }

                    response.WriteBody(documents);
                }
            });
        }

        public static void updateDocument([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                string docId = postBody["id"].ToString();
                Dictionary<string, Object> data = (Dictionary<string, Object>)postBody["data"];
                MutableDocument UpdateDoc = collection.GetDocument(docId).ToMutable();
                Dictionary<string, Object> new_data = SetDataBlob(data);
                UpdateDoc.SetData(new_data);
                collection.Save(UpdateDoc);
                response.WriteEmptyBody();
            });
            response.WriteEmptyBody();
        }

        public static void deleteDocument([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                With<Document>(postBody, "document", document =>
                {
                    collection.Delete(document);
                });
            });
            response.WriteEmptyBody();
        }

        public static void purgeDocument([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                With<Document>(postBody, "document", document =>
                {
                    collection.Purge(document);
                });
            });
            response.WriteEmptyBody();
        }

        public static void purgeDocumentById([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                string docId = postBody["docId"].ToString();
                collection.Purge(docId);
            });
            response.WriteEmptyBody();
        }

        public static void saveDocuments([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            var docDict = (Dictionary<string, object>)postBody["documents"];
            With<Database>(postBody, "database", db =>
            {
                With<Collection>(postBody, "collection", collection =>
                {
                    db.InBatch(() =>
                    {
                        foreach (var body in docDict)
                        {
                            string id = body.Key;
                            var docBody = (Dictionary<string, object>)body.Value;
                            docBody.Remove("_id");
                            MutableDocument doc = new MutableDocument(id, docBody);
                            Dictionary<string, Object> new_data = SetDataBlob(docBody);
                            collection.Save(doc);

                        }
                    });
                }); 
            });
            response.WriteEmptyBody();
        }

        public static void getDocumentExpiration([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                string docId = postBody["docId"].ToString();
                response.WriteBody(collection.GetDocumentExpiration(docId));
            });
        }

        public static void getMutableDocument([NotNull] NameValueCollection args,
                                            [NotNull] IReadOnlyDictionary<string, object> postBody,
                                            [NotNull] HttpListenerResponse response)
        {
            With<Collection>(postBody, "collection", collection =>
            {
                string docId = postBody["docId"].ToString();
                MutableDocument UpdateDoc = collection.GetDocument(docId).ToMutable();
                response.WriteBody(MemoryMap.Store(UpdateDoc));
            });
        }

        private static Dictionary<string, object> SetDataBlob(Dictionary<string, object> data)
        {
            if (!data.ContainsKey("_attachments"))
            {
                return data;
            }
            Dictionary<string, object> attachment_items = (Dictionary<string, object>)data["_attachments"];
            Dictionary<string, object> existingBlobItems = new Dictionary<string, object>();
            foreach (var attItem in attachment_items)
            {
                string attItemKey = attItem.Key;
                Dictionary<string, Object> attItemValue = (Dictionary<string, object>)attItem.Value;
                if (attItemValue.ContainsKey("data"))
                {
                    string contentType;
                    if (attItemKey.EndsWith(".png"))
                    {
                        contentType = "image/jpeg";
                    }
                    else
                        contentType = "text/plain";
                    var image = File.ReadAllBytes(attItemKey);
                    Blob blob = new Blob(contentType, image);
                    data.Add(attItemKey, blob);
                }
                else if (attItemValue.ContainsKey("digest"))
                {
                    existingBlobItems.Add(attItemKey, attItemValue);
                }
                if (existingBlobItems.Count > 0 && existingBlobItems.Count < attachment_items.Count)
                {
                    data.Remove("_attachments");
                    data.Add("_attachments", existingBlobItems);
                }
            }
            return data;
        }
    }
}