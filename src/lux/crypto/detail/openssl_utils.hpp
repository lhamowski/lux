#pragma once

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace lux::crypto::detail {

std::string get_openssl_error();

inline auto* as_uint8_ptr(std::byte* ptr)
{
    return reinterpret_cast<std::uint8_t*>(ptr);
}

inline const auto* as_uint8_ptr(const std::byte* ptr)
{
    return reinterpret_cast<const std::uint8_t*>(ptr);
}

struct evp_pkey_deleter
{
    void operator()(EVP_PKEY* pkey) const
    {
        if (pkey)
        {
            EVP_PKEY_free(pkey);
        }
    }
};

using evp_pkey_ptr = std::unique_ptr<EVP_PKEY, evp_pkey_deleter>;

struct evp_pkey_ctx_deleter
{
    void operator()(EVP_PKEY_CTX* ctx) const
    {
        if (ctx)
        {
            EVP_PKEY_CTX_free(ctx);
        }
    }
};

using evp_pkey_ctx_ptr = std::unique_ptr<EVP_PKEY_CTX, evp_pkey_ctx_deleter>;

struct bio_deleter
{
    void operator()(BIO* bio) const
    {
        if (bio)
        {
            BIO_free(bio);
        }
    }
};

using bio_ptr = std::unique_ptr<BIO, bio_deleter>;

struct x509_req_deleter
{
    void operator()(X509_REQ* req) const
    {
        if (req)
        {
            X509_REQ_free(req);
        }
    }
};

using x509_req_ptr = std::unique_ptr<X509_REQ, x509_req_deleter>;

struct x509_name_deleter
{
    void operator()(X509_NAME* name) const
    {
        if (name)
        {
            X509_NAME_free(name);
        }
    }
};

using x509_name_ptr = std::unique_ptr<X509_NAME, x509_name_deleter>;

} // namespace lux::crypto::detail