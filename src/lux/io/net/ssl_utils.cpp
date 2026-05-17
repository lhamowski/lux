#include <lux/io/net/ssl_utils.hpp>

#ifdef _WIN32
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <wincrypt.h>
#endif

namespace lux::net {

void set_default_ssl_verify_paths(lux::net::base::ssl_context& ssl_context)
{
#ifdef _WIN32
    HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
    if (hStore != nullptr)
    {
        X509_STORE* store = X509_STORE_new();
        if (store != nullptr)
        {
            PCCERT_CONTEXT pContext = NULL;
            while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL)
            {
                const auto* encoded = reinterpret_cast<const unsigned char*>(pContext->pbCertEncoded);
                X509* x509 = d2i_X509(NULL, &encoded, pContext->cbCertEncoded);
                if (x509 != NULL)
                {
                    X509_STORE_add_cert(store, x509);
                    X509_free(x509);
                }
            }

            SSL_CTX_set_cert_store(ssl_context.native_handle(), store);
        }

        CertCloseStore(hStore, 0);
    }
#endif

    ssl_context.set_default_verify_paths();
}

} // namespace lux::net
