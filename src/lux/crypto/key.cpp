#include <lux/crypto/key.hpp>
#include <lux/support/move.hpp>

#include <openssl/evp.h>
#include <openssl/err.h>

#include <memory>

namespace lux::crypto {

namespace {

std::string get_openssl_error()
{
    unsigned long err = ERR_get_error();
    if (err == 0)
    {
        return "Unknown OpenSSL error";
    }

    char buffer[256];
    ERR_error_string_n(err, buffer, sizeof(buffer));
    return std::string{buffer};
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

} // anonymous namespace

lux::result<ed25519_keypair> generate_ed25519_keypair()
{
    evp_pkey_ctx_ptr ctx{EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr)};
    if (!ctx)
    {
        return lux::err("Failed to create EVP_PKEY_CTX (err={})", get_openssl_error());
    }

    if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
    {
        return lux::err("Failed to initialize key generation (err={})", get_openssl_error());
    }

    EVP_PKEY* raw_pkey = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &raw_pkey) <= 0)
    {
        return lux::err("Failed to generate ED25519 keypair (err={})", get_openssl_error());
    }

    const evp_pkey_ptr pkey{raw_pkey};
    ed25519_keypair keypair;

    // Extract private key
    std::size_t private_key_len = 32;
    keypair.private_key.resize(private_key_len);

    auto* private_key_data = reinterpret_cast<uint8_t*>(keypair.private_key.data());
    if (EVP_PKEY_get_raw_private_key(pkey.get(), private_key_data, &private_key_len) <= 0)
    {
        return lux::err("Failed to extract private key (err={})", get_openssl_error());
    }

    keypair.private_key.resize(private_key_len);

    // Extract public key
    std::size_t public_key_len = 32;
    keypair.public_key.resize(public_key_len);

    auto* public_key_data = reinterpret_cast<uint8_t*>(keypair.public_key.data());

    if (EVP_PKEY_get_raw_public_key(pkey.get(), public_key_data, &public_key_len) <= 0)
    {
        return lux::err("Failed to extract public key (err={})", get_openssl_error());
    }

    keypair.public_key.resize(public_key_len);

    return lux::ok(lux::move(keypair));
}

} // namespace lux::crypto