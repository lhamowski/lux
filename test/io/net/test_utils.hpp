#pragma once

#include <lux/io/net/base/endpoint.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include <catch2/catch_all.hpp>

#include <cstdint>

namespace lux::test::net {

inline boost::asio::ip::tcp::endpoint make_localhost_endpoint(std::uint16_t port)
{
    return boost::asio::ip::tcp::endpoint{boost::asio::ip::make_address("127.0.0.1"), port};
}

inline boost::asio::ssl::context create_ssl_client_context()
{
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
    ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2);
    ctx.set_verify_mode(boost::asio::ssl::verify_none);
    return ctx;
}

inline boost::asio::ssl::context create_ssl_server_context()
{
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_server};
    ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                    boost::asio::ssl::context::single_dh_use);

    EVP_PKEY* pkey = EVP_PKEY_new();
    REQUIRE(pkey != nullptr);

    RSA* rsa = RSA_new();
    BIGNUM* e = BN_new();
    BN_set_word(e, RSA_F4);
    REQUIRE(RSA_generate_key_ex(rsa, 512, e, nullptr)); // 512 bits for testing purposes only
    BN_free(e);

    REQUIRE(EVP_PKEY_assign_RSA(pkey, rsa));

    X509* x509 = X509_new();
    REQUIRE(x509 != nullptr);

    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);

    X509_set_pubkey(x509, pkey);

    X509_NAME* name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name,
                               "CN",
                               MBSTRING_ASC,
                               reinterpret_cast<const unsigned char*>("localhost"),
                               -1,
                               -1,
                               0);
    X509_set_issuer_name(x509, name);

    REQUIRE(X509_sign(x509, pkey, EVP_sha256()));

    BIO* cert_bio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(cert_bio, x509);
    BUF_MEM* cert_mem;
    BIO_get_mem_ptr(cert_bio, &cert_mem);

    BIO* key_bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(key_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    BUF_MEM* key_mem;
    BIO_get_mem_ptr(key_bio, &key_mem);

    ctx.use_certificate_chain(boost::asio::buffer(cert_mem->data, cert_mem->length));
    ctx.use_private_key(boost::asio::buffer(key_mem->data, key_mem->length), boost::asio::ssl::context::pem);

    BIO_free(cert_bio);
    BIO_free(key_bio);
    X509_free(x509);
    EVP_PKEY_free(pkey);

    return ctx;
}

} // namespace lux::test::net
