﻿// 
//  BlobMethods.cs
// 
//  Author:
//   Sridevi Saragadam  <sridevi.saragadam@couchbase.com>
// 
//  Copyright (c) 2019 Couchbase, Inc All rights reserved.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
// 
//  http://www.apache.org/licenses/LICENSE-2.0
// 
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// 

using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Net;
using System.IO;
using System.Text;
using System.Runtime.Serialization.Formatters.Binary;

using static Couchbase.Lite.Blob;

using JetBrains.Annotations;

using static Couchbase.Lite.Testing.DatabaseMethods;

namespace Couchbase.Lite.Testing
{
    internal static class BlobMethods
    {
        #region Public Methods
        public static void Create([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            var contentType = postBody["contentType"].ToString();
            if (postBody.ContainsKey("content"))
            {
                byte[] byteArray = MemoryMap.Get<byte[]>(postBody["content"].ToString());
                response.WriteBody(MemoryMap.New<Blob>(contentType, byteArray));
            }
            else if (postBody.ContainsKey("stream"))
            {
                Stream stream = MemoryMap.Get<Stream>(postBody["stream"].ToString());
                response.WriteBody(MemoryMap.New<Blob>(contentType, stream));
            }

            else if (postBody.ContainsKey("fileURL"))
            {
                Uri fileUri = MemoryMap.Get<Uri>(postBody["fileURL"].ToString());
                response.WriteBody(MemoryMap.New<Blob>(contentType, fileUri));
            }
            #endregion
        }

        public static void CreateUTFBytesContent([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            var content = postBody["content"].ToString();
            var bytes = Encoding.UTF8.GetBytes(content);
            response.WriteBody(MemoryMap.Store(bytes));
        }

        public static void CreateImageStream([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            string imageLocation = TestServer.FilePathResolver(postBody["image"].ToString(), false);
            var image = File.ReadAllBytes(imageLocation);
            MemoryStream stream = new MemoryStream(image);
            response.WriteBody(MemoryMap.Store(stream));
        }

        public static void CreateImageContent([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            string imageLocation = TestServer.FilePathResolver(postBody["image"].ToString(), false);
            var image = File.ReadAllBytes(imageLocation);
            MemoryStream stream = new MemoryStream(image);
            response.WriteBody(MemoryMap.Store(stream.ToArray()));
        }

        public static void CreateImageFileUrl([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            string imageLocation = TestServer.FilePathResolver(postBody["image"].ToString(), false);
            
            string curLocation = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            Uri fileUrl = new Uri(Path.Combine(curLocation, imageLocation));
            
            response.WriteBody(MemoryMap.Store(fileUrl));
        }

        public static void Digest([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            With<Blob>(postBody, "blob", blob => response.WriteBody(MemoryMap.Store(blob.Digest)));
        }

        public static void Equals([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            var blob = postBody["blob"];
            var obj = postBody["obj"];
            response.WriteBody(blob.Equals(obj));
        }

        public static void HashCode([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            With<Blob>(postBody, "blob", blob => response.WriteBody(MemoryMap.Store(blob.GetHashCode())));
        }

        public static void GetContent([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            With<Blob>(postBody, "blob", blob => response.WriteBody(MemoryMap.Store(blob.Content)));
        }

        public static void GetProperties([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            With<Blob>(postBody, "blob", blob => response.WriteBody(MemoryMap.Store(blob.Properties)));
        }

        public static void GetContentStream([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            With<Blob>(postBody, "blob", blob => response.WriteBody(MemoryMap.Store(blob.ContentStream)));
        }

        public static void GetContentType([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            With<Blob>(postBody, "blob", blob => response.WriteBody(MemoryMap.Store(blob.ContentType)));
        }

        public static void Length([NotNull] NameValueCollection args,
            [NotNull] IReadOnlyDictionary<string, object> postBody,
            [NotNull] HttpListenerResponse response)
        {
            With<Blob>(postBody, "blob", blob => response.WriteBody(MemoryMap.Store(blob.Length)));
        }
    }
}