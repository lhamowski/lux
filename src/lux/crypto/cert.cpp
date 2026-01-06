#include <lux/crypto/cert.hpp>
#include <lux/support/move.hpp>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

#include <cstdint>
#include <cstring>
#include <memory>

namespace lux::crypto {

namespace {

std::string get_openssl_error()
{
    const unsigned long err = ERR_get_error();
    if (err == 0)
    {
        return "Unknown OpenSSL error";
    }

    char buffer[256];
    ERR_error_string_n(err, buffer, sizeof(buffer));
    return std::string{buffer};
}

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

struct bio_deleter
{
    void operator()(BIO* bio) const
    {
        if (bio)
        {
            BIO_free_all(bio);
        }
    }
};

using bio_ptr = std::unique_ptr<BIO, bio_deleter>;

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

const std::uint8_t* as_uint8_ptr(const auto* ptr)
{
    return reinterpret_cast<const std::uint8_t*>(ptr);
}

lux::result<X509_NAME*> create_subject_name(const subject_info& subject)
{
    x509_name_ptr name{X509_NAME_new()};
    if (!name)
    {
        return lux::err("Failed to create X509_NAME (err={})", get_openssl_error());
    }

    if (subject.country.has_value())
    {
        if (!X509_NAME_add_entry_by_txt(name.get(),
                                        "C",
                                        MBSTRING_ASC,
                                        as_uint8_ptr(subject.country->c_str()),
                                        -1,
                                        -1,
                                        0))
        {
            return lux::err("Failed to add country to subject (err={})", get_openssl_error());
        }
    }

    if (subject.state.has_value())
    {
        if (!X509_NAME_add_entry_by_txt(name.get(),
                                        "ST",
                                        MBSTRING_ASC,
                                        as_uint8_ptr(subject.state->c_str()),
                                        -1,
                                        -1,
                                        0))
        {
            return lux::err("Failed to add state to subject (err={})", get_openssl_error());
        }
    }

    if (subject.locality.has_value())
    {
        if (!X509_NAME_add_entry_by_txt(name.get(),
                                        "L",
                                        MBSTRING_ASC,
                                        as_uint8_ptr(subject.locality->c_str()),
                                        -1,
                                        -1,
                                        0))
        {
            return lux::err("Failed to add locality to subject (err={})", get_openssl_error());
        }
    }

    if (subject.organization.has_value())
    {
        if (!X509_NAME_add_entry_by_txt(name.get(),
                                        "O",
                                        MBSTRING_ASC,
                                        as_uint8_ptr(subject.organization->c_str()),
                                        -1,
                                        -1,
                                        0))
        {
            return lux::err("Failed to add organization to subject (err={})", get_openssl_error());
        }
    }

    if (subject.organizational_unit.has_value())
    {
        if (!X509_NAME_add_entry_by_txt(name.get(),
                                        "OU",
                                        MBSTRING_ASC,
                                        as_uint8_ptr(subject.organizational_unit->c_str()),
                                        -1,
                                        -1,
                                        0))
        {
            return lux::err("Failed to add organizational unit to subject (err={})", get_openssl_error());
        }
    }

    if (!X509_NAME_add_entry_by_txt(name.get(),
                                    "CN",
                                    MBSTRING_ASC,
                                    as_uint8_ptr(subject.common_name.c_str()),
                                    -1,
                                    -1,
                                    0))
    {
        return lux::err("Failed to add common name to subject (err={})", get_openssl_error());
    }

    if (subject.email.has_value())
    {
        if (!X509_NAME_add_entry_by_txt(name.get(),
                                        "emailAddress",
                                        MBSTRING_ASC,
                                        as_uint8_ptr(subject.email->c_str()),
                                        -1,
                                        -1,
                                        0))
        {
            return lux::err("Failed to add email to subject (err={})", get_openssl_error());
        }
    }

    return lux::ok(name.release());
}

lux::result<evp_pkey_ptr> create_evp_pkey_from_keypair(const ed25519_keypair& keypair)
{
    if (keypair.private_key.size() != 32)
    {
        return lux::err("Invalid ED25519 private key size (expected=32, actual={})", keypair.private_key.size());
    }

    if (keypair.public_key.size() != 32)
    {
        return lux::err("Invalid ED25519 public key size (expected=32, actual={})", keypair.public_key.size());
    }

    evp_pkey_ptr pkey{EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,
                                                   nullptr,
                                                   as_uint8_ptr(keypair.private_key.data()),
                                                   keypair.private_key.size())};

    if (!pkey)
    {
        return lux::err("Failed to create EVP_PKEY from ED25519 private key (err={})", get_openssl_error());
    }

    return lux::ok(lux::move(pkey));
}

} // anonymous namespace

lux::result<csr_result> generate_csr(const ed25519_keypair& keypair, const subject_info& subject, csr_format format)
{
    if (subject.common_name.empty())
    {
        return lux::err("Common name (CN) is required for CSR generation");
    }

    auto pkey_result = create_evp_pkey_from_keypair(keypair);
    if (!pkey_result)
    {
        return lux::err("Failed to create EVP_PKEY: {}", pkey_result.error().str());
    }
    evp_pkey_ptr pkey = lux::move(pkey_result.value());

    x509_req_ptr req{X509_REQ_new()};
    if (!req)
    {
        return lux::err("Failed to create X509_REQ (err={})", get_openssl_error());
    }

    if (!X509_REQ_set_version(req.get(), 0))
    {
        return lux::err("Failed to set CSR version (err={})", get_openssl_error());
    }

    auto subject_name_result = create_subject_name(subject);
    if (!subject_name_result)
    {
        return lux::err("Failed to create subject name: {}", subject_name_result.error().str());
    }
    X509_NAME* const subject_name = subject_name_result.value();

    if (!X509_REQ_set_subject_name(req.get(), subject_name))
    {
        X509_NAME_free(subject_name);
        return lux::err("Failed to set subject name (err={})", get_openssl_error());
    }
    X509_NAME_free(subject_name);

    if (!X509_REQ_set_pubkey(req.get(), pkey.get()))
    {
        return lux::err("Failed to set public key (err={})", get_openssl_error());
    }

    if (!X509_REQ_sign(req.get(), pkey.get(), nullptr))
    {
        return lux::err("Failed to sign CSR (err={})", get_openssl_error());
    }

    bio_ptr bio{BIO_new(BIO_s_mem())};
    if (!bio)
    {
        return lux::err("Failed to create BIO (err={})", get_openssl_error());
    }

    if (format == csr_format::pem)
    {
        if (!PEM_write_bio_X509_REQ(bio.get(), req.get()))
        {
            return lux::err("Failed to write CSR in PEM format (err={})", get_openssl_error());
        }
    }
    else
    {
        if (!i2d_X509_REQ_bio(bio.get(), req.get()))
        {
            return lux::err("Failed to write CSR in DER format (err={})", get_openssl_error());
        }
    }

    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio.get(), &mem);
    if (!mem || !mem->data || mem->length == 0)
    {
        return lux::err("Failed to get CSR data from BIO");
    }

    csr_result result;
    result.format = format;
    result.data.resize(mem->length);
    std::memcpy(result.data.data(), mem->data, mem->length);

    return lux::ok(lux::move(result));
}

} // namespace lux::crypto
