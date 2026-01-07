#include <lux/crypto/cert.hpp>
#include <lux/crypto/detail/openssl_utils.hpp>
#include <lux/support/move.hpp>

#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/asn1.h>

#include <cstdint>
#include <cstring>
#include <memory>

namespace lux::crypto {

namespace {

lux::result<void> add_name_entry(X509_NAME* name, const char* field, const std::string& value)
{
    if (!value.empty())
    {
        if (X509_NAME_add_entry_by_txt(name,
                                       field,
                                       MBSTRING_UTF8,
                                       reinterpret_cast<const unsigned char*>(value.c_str()),
                                       -1,
                                       -1,
                                       0) != 1)
        {
            return lux::err("Failed to add {} to X509_NAME (err={})", field, detail::get_openssl_error());
        }
    }
    return lux::ok();
}

lux::result<X509_NAME*> create_subject_name(const subject_info& subject)
{
    detail::x509_name_ptr name{X509_NAME_new()};
    if (!name)
    {
        return lux::err("Failed to create X509_NAME (err={})", detail::get_openssl_error());
    }

    if (auto res = add_name_entry(name.get(), "CN", subject.common_name); !res)
    {
        return lux::err(lux::move(res.error()));
    }

    if (subject.country)
    {
        if (auto res = add_name_entry(name.get(), "C", *subject.country); !res)
        {
            return lux::err(lux::move(res.error()));
        }
    }

    if (subject.state)
    {
        if (auto res = add_name_entry(name.get(), "ST", *subject.state); !res)
        {
            return lux::err(lux::move(res.error()));
        }
    }

    if (subject.locality)
    {
        if (auto res = add_name_entry(name.get(), "L", *subject.locality); !res)
        {
            return lux::err(lux::move(res.error()));
        }
    }

    if (subject.organization)
    {
        if (auto res = add_name_entry(name.get(), "O", *subject.organization); !res)
        {
            return lux::err(lux::move(res.error()));
        }
    }

    if (subject.organizational_unit)
    {
        if (auto res = add_name_entry(name.get(), "OU", *subject.organizational_unit); !res)
        {
            return lux::err(lux::move(res.error()));
        }
    }

    if (subject.email)
    {
        if (auto res = add_name_entry(name.get(), "emailAddress", *subject.email); !res)
        {
            return lux::err(lux::move(res.error()));
        }
    }

    return name.release();
}

} // anonymous namespace

lux::result<csr_der> generate_csr(const lux::crypto::ed25519_private_key& private_key, const subject_info& subject)
{
    detail::evp_pkey_ptr pkey{EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,
                                                           nullptr,
                                                           detail::as_uint8_ptr(private_key.data.data()),
                                                           private_key.data.size())};
    if (!pkey)
    {
        return lux::err("Failed to create EVP_PKEY from private key (err={})", detail::get_openssl_error());
    }

    detail::x509_req_ptr req{X509_REQ_new()};
    if (!req)
    {
        return lux::err("Failed to create X509_REQ (err={})", detail::get_openssl_error());
    }

    if (X509_REQ_set_version(req.get(), 0) != 1)
    {
        return lux::err("Failed to set X509_REQ version (err={})", detail::get_openssl_error());
    }

    auto name_result = create_subject_name(subject);
    if (!name_result)
    {
        return lux::err(lux::move(name_result.error()));
    }

    X509_NAME* name{*name_result};
    if (X509_REQ_set_subject_name(req.get(), name) != 1)
    {
        X509_NAME_free(name);
        return lux::err("Failed to set subject name (err={})", detail::get_openssl_error());
    }
    X509_NAME_free(name);

    if (X509_REQ_set_pubkey(req.get(), pkey.get()) != 1)
    {
        return lux::err("Failed to set public key (err={})", detail::get_openssl_error());
    }

    if (!subject.subject_alt_names.empty())
    {
        GENERAL_NAMES* gens{sk_GENERAL_NAME_new_null()};
        if (!gens)
        {
            return lux::err("Failed to create GENERAL_NAMES (err={})", detail::get_openssl_error());
        }

        for (const auto& san : subject.subject_alt_names)
        {
            GENERAL_NAME* gen{GENERAL_NAME_new()};
            if (!gen)
            {
                sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
                return lux::err("Failed to create GENERAL_NAME (err={})", detail::get_openssl_error());
            }

            ASN1_IA5STRING* ia5{ASN1_IA5STRING_new()};
            if (!ia5)
            {
                GENERAL_NAME_free(gen);
                sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
                return lux::err("Failed to create ASN1_IA5STRING (err={})", detail::get_openssl_error());
            }

            if (ASN1_STRING_set(ia5, san.c_str(), static_cast<int>(san.length())) != 1)
            {
                ASN1_IA5STRING_free(ia5);
                GENERAL_NAME_free(gen);
                sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
                return lux::err("Failed to set ASN1_IA5STRING (err={})", detail::get_openssl_error());
            }

            gen->type = GEN_DNS;
            gen->d.dNSName = ia5;

            if (sk_GENERAL_NAME_push(gens, gen) == 0)
            {
                GENERAL_NAME_free(gen);
                sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
                return lux::err("Failed to add GENERAL_NAME (err={})", detail::get_openssl_error());
            }
        }

        X509_EXTENSIONS* exts{sk_X509_EXTENSION_new_null()};
        if (!exts)
        {
            sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
            return lux::err("Failed to create X509_EXTENSIONS (err={})", detail::get_openssl_error());
        }

        X509_EXTENSION* ext{X509V3_EXT_i2d(NID_subject_alt_name, 0, gens)};
        sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);

        if (!ext)
        {
            sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
            return lux::err("Failed to create SAN extension (err={})", detail::get_openssl_error());
        }

        if (sk_X509_EXTENSION_push(exts, ext) == 0)
        {
            X509_EXTENSION_free(ext);
            sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
            return lux::err("Failed to add SAN extension (err={})", detail::get_openssl_error());
        }

        if (X509_REQ_add_extensions(req.get(), exts) != 1)
        {
            sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
            return lux::err("Failed to add extensions to CSR (err={})", detail::get_openssl_error());
        }

        sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
    }

    if (X509_REQ_sign(req.get(), pkey.get(), nullptr) <= 0)
    {
        return lux::err("Failed to sign CSR (err={})", detail::get_openssl_error());
    }

    unsigned char* der_data{nullptr};
    int der_len{i2d_X509_REQ(req.get(), &der_data)};
    if (der_len < 0)
    {
        return lux::err("Failed to encode CSR to DER (err={})", detail::get_openssl_error());
    }

    const std::byte* der_data_as_bytes = reinterpret_cast<std::byte*>(der_data);
    std::vector<std::byte> der_vec{der_data_as_bytes, der_data_as_bytes + der_len};
    OPENSSL_free(der_data);

    return csr_der{lux::move(der_vec)};
}

lux::result<csr_pem> to_pem(const csr_der& der_csr)
{
    const unsigned char* der_ptr{detail::as_uint8_ptr(const_cast<std::byte*>(der_csr.get().data()))};
    X509_REQ* req{d2i_X509_REQ(nullptr, &der_ptr, static_cast<long>(der_csr.get().size()))};
    if (!req)
    {
        return lux::err("Failed to decode DER CSR (err={})", detail::get_openssl_error());
    }
    detail::x509_req_ptr req_ptr{req};

    detail::bio_ptr bio{BIO_new(BIO_s_mem())};
    if (!bio)
    {
        return lux::err("Failed to create memory BIO (err={})", detail::get_openssl_error());
    }

    if (PEM_write_bio_X509_REQ(bio.get(), req_ptr.get()) != 1)
    {
        return lux::err("Failed to write CSR to PEM (err={})", detail::get_openssl_error());
    }

    char* data{nullptr};
    long len{BIO_get_mem_data(bio.get(), &data)};
    if (len <= 0)
    {
        return lux::err("Failed to get PEM data from BIO");
    }

    return csr_pem{std::string{data, static_cast<std::size_t>(len)}};
}

} // namespace lux::crypto
