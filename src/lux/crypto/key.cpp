#include <lux/crypto/key.hpp>

#include <lux/crypto/detail/openssl_utils.hpp>
#include <lux/support/move.hpp>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include <memory>

namespace lux::crypto {

lux::result<ed25519_private_key> generate_ed25519_private_key()
{
    detail::evp_pkey_ctx_ptr ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr)};
    if (!ctx)
    {
        return lux::err("Failed to create EVP_PKEY_CTX (err={})", detail::get_openssl_error());
    }

    if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
    {
        return lux::err("Failed to initialize key generation (err={})", detail::get_openssl_error());
    }

    EVP_PKEY* raw_pkey{nullptr};
    if (EVP_PKEY_keygen(ctx.get(), &raw_pkey) <= 0)
    {
        return lux::err("Failed to generate Ed25519 key (err={})", detail::get_openssl_error());
    }

    detail::evp_pkey_ptr pkey{raw_pkey};

    std::size_t key_len{ed25519_private_key::size};
    ed25519_private_key private_key{};

    if (EVP_PKEY_get_raw_private_key(pkey.get(), detail::as_uint8_ptr(private_key.data.data()), &key_len) <= 0)
    {
        return lux::err("Failed to extract Ed25519 private key (err={})", detail::get_openssl_error());
    }

    if (key_len != ed25519_private_key::size)
    {
        return lux::err("Invalid Ed25519 private key size (expected={}, actual={})",
                        ed25519_private_key::size,
                        key_len);
    }

    return private_key;
}

lux::result<ed25519_public_key> derive_public_key(const ed25519_private_key& private_key)
{
    detail::evp_pkey_ptr pkey{EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,
                                                           nullptr,
                                                           detail::as_uint8_ptr(private_key.data.data()),
                                                           private_key.data.size())};
    if (!pkey)
    {
        return lux::err("Failed to create EVP_PKEY from private key (err={})", detail::get_openssl_error());
    }

    ed25519_public_key public_key{};
    std::size_t key_len{ed25519_public_key::size};

    if (EVP_PKEY_get_raw_public_key(pkey.get(), detail::as_uint8_ptr(public_key.data.data()), &key_len) <= 0)
    {
        return lux::err("Failed to derive Ed25519 public key (err={})", detail::get_openssl_error());
    }

    if (key_len != ed25519_public_key::size)
    {
        return lux::err("Invalid Ed25519 public key size (expected={}, actual={})", ed25519_public_key::size, key_len);
    }

    return public_key;
}

lux::result<lux::crypto::secure_string> to_pem(const ed25519_private_key& private_key)
{
    detail::evp_pkey_ptr pkey{EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,
                                                           nullptr,
                                                           detail::as_uint8_ptr(private_key.data.data()),
                                                           private_key.data.size())};
    if (!pkey)
    {
        return lux::err("Failed to create EVP_PKEY from private key (err={})", detail::get_openssl_error());
    }

    detail::bio_ptr bio{BIO_new(BIO_s_mem())};
    if (!bio)
    {
        return lux::err("Failed to create memory BIO (err={})", detail::get_openssl_error());
    }

    if (PEM_write_bio_PrivateKey(bio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) <= 0)
    {
        return lux::err("Failed to write private key to PEM (err={})", detail::get_openssl_error());
    }

    char* data{nullptr};
    long len{BIO_get_mem_data(bio.get(), &data)};
    if (len <= 0)
    {
        return lux::err("Failed to get PEM data from BIO");
    }

    return lux::crypto::secure_string{data, static_cast<std::size_t>(len)};
}

lux::result<std::string> to_pem(const ed25519_public_key& public_key)
{
    detail::evp_pkey_ptr pkey{EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,
                                                          nullptr,
                                                          detail::as_uint8_ptr(public_key.data.data()),
                                                          public_key.data.size())};
    if (!pkey)
    {
        return lux::err("Failed to create EVP_PKEY from public key (err={})", detail::get_openssl_error());
    }

    detail::bio_ptr bio{BIO_new(BIO_s_mem())};
    if (!bio)
    {
        return lux::err("Failed to create memory BIO (err={})", detail::get_openssl_error());
    }

    if (PEM_write_bio_PUBKEY(bio.get(), pkey.get()) <= 0)
    {
        return lux::err("Failed to write public key to PEM (err={})", detail::get_openssl_error());
    }

    char* data{nullptr};
    long len{BIO_get_mem_data(bio.get(), &data)};
    if (len <= 0)
    {
        return lux::err("Failed to get PEM data from BIO");
    }

    return std::string{data, static_cast<std::size_t>(len)};
}

} // namespace lux::crypto