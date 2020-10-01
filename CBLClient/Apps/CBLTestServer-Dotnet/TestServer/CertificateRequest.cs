using System;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;

namespace TestServer
{
    internal class CertificateRequest
    {
        private string v;
        private ECDsa ecdsa;
        private HashAlgorithmName sHA256;

        public CertificateRequest(string v, ECDsa ecdsa, HashAlgorithmName sHA256)
        {
            this.v = v;
            this.ecdsa = ecdsa;
            this.sHA256 = sHA256;
        }

        internal X509Certificate CreateSelfSigned(DateTimeOffset now, DateTimeOffset dateTimeOffset)
        {
            throw new NotImplementedException();
        }
    }
}