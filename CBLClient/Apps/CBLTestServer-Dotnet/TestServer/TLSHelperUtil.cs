using System;
using System.Security;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
namespace TestServer
{
    public class TLSHelperUtil
    {
        public TLSHelperUtil()
        {

        }
        public X509Certificate CreateClientCert(string name)
        {
            var ecdsa = ECDsa.Create();
            var req = new CertificateRequest($"cn={name}", ecdsa, HashAlgorithmName.SHA256);
            return req.CreateSelfSigned(DateTimeOffset.Now, DateTimeOffset.Now.AddYears(1));
        }
    }
}
