#include "CertUtil.h"
#include "logging/Logger.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>

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
    if (!rsa) { EVP_PKEY_free(pkey); return false; }

    BIGNUM* bn = BN_new();
    if (!bn) { RSA_free(rsa); EVP_PKEY_free(pkey); return false; }

    BN_set_word(bn, RSA_F4);
    if (RSA_generate_key_ex(rsa, 2048, bn, nullptr) != 1) {
        BN_free(bn); RSA_free(rsa); EVP_PKEY_free(pkey); return false;
    }
    BN_free(bn);

    if (EVP_PKEY_set1_RSA(pkey, rsa) != 1) {
        RSA_free(rsa); EVP_PKEY_free(pkey); return false;
    }
    RSA_free(rsa);
    *out_pkey = pkey;
    return true;
}

bool GenerateCertificate(X509** out_x509, EVP_PKEY* pkey) {
    *out_x509 = nullptr;
    X509* x509 = X509_new();
    if (!x509) return false;

    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    time_t now = time(nullptr);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 365 * 24 * 60 * 60);
    X509_set_pubkey(x509, pkey);

    X509_NAME* name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
        reinterpret_cast<const unsigned char*>("127.0.0.1"), -1, -1, 0);
    X509_set_issuer_name(x509, name);

    {
        GENERAL_NAMES* sans = GENERAL_NAMES_new();
        if (sans) {
            ASN1_STRING* dns = ASN1_STRING_type_new(V_ASN1_IA5STRING);
            if (dns) {
                ASN1_STRING_set(dns, "localhost", 9);
                GENERAL_NAME* gn = GENERAL_NAME_new();
                if (gn) {
                    GENERAL_NAME_set0_value(gn, GEN_DNS, dns);
                    sk_GENERAL_NAME_push(sans, gn);
                } else { ASN1_STRING_free(dns); }
            }
            unsigned char ipv4[] = {127, 0, 0, 1};
            ASN1_OCTET_STRING* ip_str = ASN1_OCTET_STRING_new();
            if (ip_str && ASN1_OCTET_STRING_set(ip_str, ipv4, sizeof(ipv4))) {
                GENERAL_NAME* gn = GENERAL_NAME_new();
                if (gn) {
                    GENERAL_NAME_set0_value(gn, GEN_IPADD, ip_str);
                    sk_GENERAL_NAME_push(sans, gn);
                } else { ASN1_OCTET_STRING_free(ip_str); }
            }
            if (sk_GENERAL_NAME_num(sans) > 0) {
                X509V3_CTX ctx;
                X509V3_set_ctx_nodb(&ctx);
                X509V3_set_ctx(&ctx, x509, x509, nullptr, nullptr, 0);
                X509_EXTENSION* ext = X509V3_EXT_i2d(NID_subject_alt_name, 0, sans);
                if (ext) { X509_add_ext(x509, ext, -1); X509_EXTENSION_free(ext); }
            }
            sk_GENERAL_NAME_pop_free(sans, GENERAL_NAME_free);
        }
    }

    if (X509_sign(x509, pkey, EVP_sha256()) == 0) {
        X509_free(x509); return false;
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
        EVP_PKEY_free(pkey); return false;
    }

    FILE* key_fp = nullptr;
    if (fopen_s(&key_fp, key_path.c_str(), "w") != 0 || !key_fp) {
        memolog::LogError("certutil.key_file.fail", "failed to open key file", {{"path", key_path}});
        X509_free(x509); EVP_PKEY_free(pkey); return false;
    }
    if (!PEM_write_PrivateKey(key_fp, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
        memolog::LogError("certutil.key_write.fail", "failed to write private key PEM",
            {{"openssl_error", std::to_string(ERR_get_error())}});
        fclose(key_fp); X509_free(x509); EVP_PKEY_free(pkey); return false;
    }
    fclose(key_fp);

    FILE* crt_fp = nullptr;
    if (fopen_s(&crt_fp, crt_path.c_str(), "w") != 0 || !crt_fp) {
        memolog::LogError("certutil.crt_file.fail", "failed to open certificate file", {{"path", crt_path}});
        X509_free(x509); EVP_PKEY_free(pkey); return false;
    }
    if (!PEM_write_X509(crt_fp, x509)) {
        memolog::LogError("certutil.crt_write.fail", "failed to write certificate PEM",
            {{"openssl_error", std::to_string(ERR_get_error())}});
        fclose(crt_fp); X509_free(x509); EVP_PKEY_free(pkey); return false;
    }
    fclose(crt_fp);

    memolog::LogInfo("certutil.ok", "self-signed certificate and key generated",
        {{"crt", crt_path}, {"key", key_path}});
    X509_free(x509);
    EVP_PKEY_free(pkey);
    return true;
}

namespace {

bool WritePrivateKey(const std::string& key_path, EVP_PKEY* pkey) {
    FILE* key_fp = nullptr;
    if (fopen_s(&key_fp, key_path.c_str(), "w") != 0 || !key_fp) {
        memolog::LogError("certutil.key_file.fail", "failed to open key file", {{"path", key_path}});
        return false;
    }
    if (!PEM_write_PrivateKey(key_fp, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
        memolog::LogError("certutil.key_write.fail", "failed to write private key PEM",
            {{"openssl_error", std::to_string(ERR_get_error())}});
        fclose(key_fp); return false;
    }
    fclose(key_fp);
    return true;
}

bool WriteCertificate(const std::string& crt_path, X509* x509) {
    FILE* crt_fp = nullptr;
    if (fopen_s(&crt_fp, crt_path.c_str(), "w") != 0 || !crt_fp) {
        memolog::LogError("certutil.crt_file.fail", "failed to open certificate file", {{"path", crt_path}});
        return false;
    }
    if (!PEM_write_X509(crt_fp, x509)) {
        memolog::LogError("certutil.crt_write.fail", "failed to write certificate PEM",
            {{"openssl_error", std::to_string(ERR_get_error())}});
        fclose(crt_fp); return false;
    }
    fclose(crt_fp);
    return true;
}

bool WritePKCS12(const std::string& pfx_path, const std::string& password,
                 EVP_PKEY* pkey, X509* x509) {
    FILE* pfx_fp = nullptr;
    if (fopen_s(&pfx_fp, pfx_path.c_str(), "wb") != 0 || !pfx_fp) {
        memolog::LogError("certutil.pfx_file.fail", "failed to open PFX file", {{"path", pfx_path}});
        return false;
    }

    PKCS12* p12 = PKCS12_create(password.c_str(), nullptr, pkey, x509, nullptr,
                                 0, 0, -1, -1, 0);
    if (!p12) {
        memolog::LogError("certutil.pkcs12.fail", "failed to create PKCS12", {});
        fclose(pfx_fp); return false;
    }

    if (!i2d_PKCS12_fp(pfx_fp, p12)) {
        memolog::LogError("certutil.pfx_write.fail", "failed to write PFX",
            {{"openssl_error", std::to_string(ERR_get_error())}});
        PKCS12_free(p12); fclose(pfx_fp); return false;
    }

    PKCS12_free(p12);
    fclose(pfx_fp);
    return true;
}

}  // namespace

bool GenerateSelfSignedCertPfx(const std::string& pfx_path, const std::string& crt_path,
                               const std::string& key_path, const std::string& password) {
    EVP_PKEY* pkey = nullptr;
    X509* x509 = nullptr;

    if (!GenerateRSAKey(&pkey)) {
        memolog::LogError("certutil.key_gen.fail", "failed to generate RSA key", {});
        return false;
    }
    if (!GenerateCertificate(&x509, pkey)) {
        memolog::LogError("certutil.cert_gen.fail", "failed to generate certificate", {});
        EVP_PKEY_free(pkey); return false;
    }

    bool ok = WritePrivateKey(key_path, pkey);
    if (ok) ok = WriteCertificate(crt_path, x509);
    if (ok) ok = WritePKCS12(pfx_path, password, pkey, x509);

    if (ok) {
        memolog::LogInfo("certutil.pfx.ok", "self-signed PEM+PFX cert/key generated",
            {{"pfx", pfx_path}, {"crt", crt_path}, {"key", key_path}});
    }
    X509_free(x509);
    EVP_PKEY_free(pkey);
    return ok;
}

}  // namespace CertUtil
