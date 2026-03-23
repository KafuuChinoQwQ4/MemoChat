/*
 * CertUtil.cpp — Certificate generation using OpenSSL
 *
 * Uses WinCompat.h for Windows SDK isolation (byte macro fix).
 * Generates self-signed certificates for TLS/HTTP2 support.
 */

#include "WinCompat.h"
#include "CertUtil.h"
#include "logging/Logger.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/evp.h>

#include <cstring>
#include <ctime>

namespace {

bool GenerateRSAKey(EVP_PKEY** out_pkey) {
    *out_pkey = nullptr;

    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!pkey) return false;

    RSA* rsa = RSA_new();
    if (!rsa) {
        EVP_PKEY_free(pkey);
        return false;
    }

    BIGNUM* bn = BN_new();
    if (!bn) {
        RSA_free(rsa);
        EVP_PKEY_free(pkey);
        return false;
    }

    BN_set_word(bn, RSA_F4);
    int key_bits = 2048;
    if (RSA_generate_key_ex(rsa, key_bits, bn, nullptr) != 1) {
        BN_free(bn);
        RSA_free(rsa);
        EVP_PKEY_free(pkey);
        return false;
    }
    BN_free(bn);

    if (EVP_PKEY_set1_RSA(pkey, rsa) != 1) {
        RSA_free(rsa);
        EVP_PKEY_free(pkey);
        return false;
    }
    RSA_free(rsa);

    *out_pkey = pkey;
    return true;
}

bool GenerateCertificate(X509** out_x509, EVP_PKEY* pkey) {
    *out_x509 = nullptr;

    X509* x509 = X509_new();
    if (!x509) return false;

    // Set version to X509v3
    X509_set_version(x509, 2);

    // Set serial number
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    // Set validity period (1 year)
    time_t now = time(nullptr);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 365 * 24 * 60 * 60);

    // Set public key
    X509_set_pubkey(x509, pkey);

    // Set subject name
    X509_NAME* name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
        reinterpret_cast<const unsigned char*>("127.0.0.1"), -1, -1, 0);

    // Set issuer name (self-signed: same as subject)
    X509_set_issuer_name(x509, name);

    // Sign the certificate
    if (X509_sign(x509, pkey, EVP_sha256()) == 0) {
        X509_free(x509);
        return false;
    }

    *out_x509 = x509;
    return true;
}

}  // namespace

namespace CertUtil {

bool GenerateSelfSignedCertPem(const std::string& crt_path, const std::string& key_path) {
    EVP_PKEY* pkey = nullptr;
    X509* x509 = nullptr;

    // Generate RSA key pair
    if (!GenerateRSAKey(&pkey)) {
        memolog::LogError("certutil.key_gen.fail", "failed to generate RSA key", {});
        return false;
    }

    // Generate self-signed certificate
    if (!GenerateCertificate(&x509, pkey)) {
        memolog::LogError("certutil.cert_gen.fail", "failed to generate certificate", {});
        EVP_PKEY_free(pkey);
        return false;
    }

    // Write private key PEM
    {
        FILE* key_fp = nullptr;
        if (fopen_s(&key_fp, key_path.c_str(), "w") != 0 || !key_fp) {
            memolog::LogError("certutil.key_file.fail", "failed to open key file",
                {{"path", key_path}});
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }

        if (!PEM_write_PrivateKey(key_fp, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
            memolog::LogError("certutil.key_write.fail", "failed to write private key PEM",
                {{"openssl_error", std::to_string(ERR_get_error())}});
            fclose(key_fp);
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        fclose(key_fp);
    }

    // Write certificate PEM
    {
        FILE* crt_fp = nullptr;
        if (fopen_s(&crt_fp, crt_path.c_str(), "w") != 0 || !crt_fp) {
            memolog::LogError("certutil.crt_file.fail", "failed to open certificate file",
                {{"path", crt_path}});
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }

        if (!PEM_write_X509(crt_fp, x509)) {
            memolog::LogError("certutil.crt_write.fail", "failed to write certificate PEM",
                {{"openssl_error", std::to_string(ERR_get_error())}});
            fclose(crt_fp);
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        fclose(crt_fp);
    }

    X509_free(x509);
    EVP_PKEY_free(pkey);

    memolog::LogInfo("certutil.ok",
        "self-signed certificate and key generated",
        {{"crt", crt_path}, {"key", key_path}});
    return true;
}

}  // namespace CertUtil
