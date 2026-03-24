/*
 * CertUtil.cpp — Certificate generation using OpenSSL
 *
 * Uses WinCompat.h for Windows SDK isolation (byte macro fix).
 * Generates self-signed certificates for TLS/HTTP2 support.
 *
 * NOTE: On Windows with static OpenSSL, the OPENSSL_Applink CRT thunk
 * is required. GateServer's CMakeLists links OpenSSL DLL instead
 * (CMAKE_EXE_LINKER_FLAGS or vcpkg triplet x64-windows) to avoid this.
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

/* OPENSSL_Applink is NOT needed when OpenSSL is dynamically linked (DLL).
 * vcpkg's x64-windows OpenSSL uses DLL linking (libcrypto-3-x64.dll, libssl-3-x64.dll).
 * Including applink.c with DLL linking causes "OPENSSL_Uplink: no OPENSSL_Applink".
 *
 * Only include applink.c if OpenSSL is statically linked:
 *   #ifdef OPENSSL_STATIC_LINK
 *   #include <openssl/applink.c>
 *   #endif
 *
 * However, some vcpkg OpenSSL DLL builds still require the CRT thunk on Windows.
 * We include applink.c unconditionally on Windows to be safe:
 */
#ifdef _WIN32
#include <openssl/applink.c>
#endif

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

    // Add Subject Alternative Name (SAN) so Chromium-based browsers accept the cert
    {
        GENERAL_NAMES* sans = GENERAL_NAMES_new();
        if (sans) {
            // DNS: localhost
            ASN1_STRING* dns_localhost = ASN1_STRING_type_new(V_ASN1_IA5STRING);
            if (dns_localhost) {
                ASN1_STRING_set(dns_localhost, "localhost", 9);
                GENERAL_NAME* gn_dns = GENERAL_NAME_new();
                if (gn_dns) {
                    GENERAL_NAME_set0_value(gn_dns, GEN_DNS, dns_localhost);
                    sk_GENERAL_NAME_push(sans, gn_dns);
                } else {
                    ASN1_STRING_free(dns_localhost);
                }
            }
            // IP: 127.0.0.1 (IPv4 in binary: 4 bytes, network byte order)
            ASN1_OCTET_STRING* ip_octets = ASN1_OCTET_STRING_new();
            if (ip_octets) {
                unsigned char ipv4[] = {127, 0, 0, 1};
                if (ASN1_OCTET_STRING_set(ip_octets, ipv4, sizeof(ipv4))) {
                    GENERAL_NAME* gn_ip = GENERAL_NAME_new();
                    if (gn_ip) {
                        GENERAL_NAME_set0_value(gn_ip, GEN_IPADD, ip_octets);
                        sk_GENERAL_NAME_push(sans, gn_ip);
                    } else {
                        ASN1_OCTET_STRING_free(ip_octets);
                    }
                } else {
                    ASN1_OCTET_STRING_free(ip_octets);
                }
            }
            if (sk_GENERAL_NAME_num(sans) > 0) {
                X509V3_CTX ctx;
                X509V3_set_ctx_nodb(&ctx);
                X509V3_set_ctx(&ctx, x509, x509, nullptr, nullptr, 0);
                X509_EXTENSION* ext = X509V3_EXT_i2d(NID_subject_alt_name, 0, sans);
                if (ext) {
                    X509_add_ext(x509, ext, -1);
                    X509_EXTENSION_free(ext);
                }
            }
            sk_GENERAL_NAME_pop_free(sans, GENERAL_NAME_free);
        }
    }

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

    if (!GenerateRSAKey(&pkey)) {
        memolog::LogError("certutil.key_gen.fail", "failed to generate RSA key", {});
        return false;
    }

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

    memolog::LogInfo("certutil.ok",
        "self-signed certificate and key generated",
        {{"crt", crt_path}, {"key", key_path}});

    X509_free(x509);
    EVP_PKEY_free(pkey);
    return true;
}

bool GenerateSelfSignedCertPfx(const std::string& pfx_path,
                               const std::string& crt_path,
                               const std::string& key_path,
                               const std::string& password) {
    EVP_PKEY* pkey = nullptr;
    X509* x509 = nullptr;

    if (!GenerateRSAKey(&pkey)) {
        memolog::LogError("certutil.pfx.key_gen.fail", "failed to generate RSA key", {});
        return false;
    }

    if (!GenerateCertificate(&x509, pkey)) {
        memolog::LogError("certutil.pfx.cert_gen.fail", "failed to generate certificate", {});
        EVP_PKEY_free(pkey);
        return false;
    }

    // Write private key PEM (for QUIC_CERTIFICATE_FILE_PROTECTED)
    {
        FILE* key_fp = nullptr;
        if (fopen_s(&key_fp, key_path.c_str(), "w") != 0 || !key_fp) {
            memolog::LogError("certutil.pfx.key_file.fail",
                "failed to open key file", {{"path", key_path}});
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        if (!PEM_write_PrivateKey(key_fp, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
            memolog::LogError("certutil.pfx.key_write.fail",
                "failed to write private key PEM",
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
            memolog::LogError("certutil.pfx.crt_file.fail",
                "failed to open certificate file", {{"path", crt_path}});
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        if (!PEM_write_X509(crt_fp, x509)) {
            memolog::LogError("certutil.pfx.crt_write.fail",
                "failed to write certificate PEM",
                {{"openssl_error", std::to_string(ERR_get_error())}});
            fclose(crt_fp);
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        fclose(crt_fp);
    }

    // Build a certificate chain (just self-signed for now)
    STACK_OF(X509)* ca_stack = sk_X509_new_null();
    if (!ca_stack) {
        memolog::LogError("certutil.pfx.ca_stack.fail", "failed to create CA stack", {});
        X509_free(x509);
        EVP_PKEY_free(pkey);
        return false;
    }
    sk_X509_push(ca_stack, x509);

    // Write PFX / PKCS12 file
    PKCS12* p12 = PKCS12_create(
        password.c_str(),   // niam (reverse of "main" — used as key name in some implementations)
        "memochat",        // friendly name
        pkey,              // private key
        x509,              // certificate
        ca_stack,          // CA stack (may be nullptr)
        0,                 // default NID for key encryption (3DES)
        0,                 // default NID for cert encryption (RC2)
        0,                 // iter count for key (default 2048)
        0,                 // iter count for MAC (default 1)
        0                  // key usage flags
    );
    sk_X509_pop_free(ca_stack, X509_free);
    X509_free(x509);
    EVP_PKEY_free(pkey);

    if (!p12) {
        memolog::LogError("certutil.pfx.create.fail",
            "PKCS12_create failed",
            {{"openssl_error", std::to_string(ERR_get_error())}});
        return false;
    }

    {
        FILE* pfx_fp = nullptr;
        if (fopen_s(&pfx_fp, pfx_path.c_str(), "wb") != 0 || !pfx_fp) {
            memolog::LogError("certutil.pfx.file.fail",
                "failed to open PFX file", {{"path", pfx_path}});
            PKCS12_free(p12);
            return false;
        }
        if (!i2d_PKCS12_fp(pfx_fp, p12)) {
            memolog::LogError("certutil.pfx.write.fail",
                "failed to write PFX file",
                {{"openssl_error", std::to_string(ERR_get_error())}});
            fclose(pfx_fp);
            PKCS12_free(p12);
            return false;
        }
        fclose(pfx_fp);
    }

    PKCS12_free(p12);
    memolog::LogInfo("certutil.pfx.ok",
        "self-signed certificate bundle generated (PFX + PEM)",
        {{"pfx", pfx_path}, {"crt", crt_path}, {"key", key_path}});
    return true;
}

}  // namespace CertUtil
