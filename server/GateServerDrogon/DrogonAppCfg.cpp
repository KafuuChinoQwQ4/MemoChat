#include "WinCompat.h"
#include "DrogonAppCfg.h"
#include "CertUtil.h"
#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>
#include <filesystem>

using namespace drogon;

namespace {

bool EnsureSelfSignedCert(std::string& crt_path, std::string& key_path) {
    std::filesystem::path exe_dir = std::filesystem::current_path();
    crt_path = (exe_dir / "server.crt").string();
    key_path = (exe_dir / "server.key").string();

    if (std::filesystem::exists(crt_path) && std::filesystem::exists(key_path)) {
        return true;
    }

    return CertUtil::GenerateSelfSignedCertPem(crt_path, key_path);
}

}  // namespace

void DrogonAppCfg::Configure()
{
    app().setThreadNum(4);
    app().setUploadPath("./uploads");
    app().setLogLevel(trantor::Logger::LogLevel::kInfo);
    app().setMaxConnectionNum(100000);

    // Enable HTTP/2 over TLS with a self-signed certificate.
    // HTTP/2 requires TLS with ALPN negotiation.
    // setSSLFiles() configures the SSL certificate, then addListener() uses it.
    std::string crt_path, key_path;
    if (EnsureSelfSignedCert(crt_path, key_path)) {
        try {
            app().setSSLFiles(crt_path, key_path);
            app().addListener("0.0.0.0", 8443, true);
            LOG_INFO << "GateServerDrogon: HTTPS/HTTP2 enabled on port 8443";
            LOG_INFO << "GateServerDrogon: TLS cert=" << crt_path;
            LOG_INFO << "GateServerDrogon: TLS key=" << key_path;
        } catch (const std::exception& ex) {
            LOG_ERROR << "GateServerDrogon: failed to enable HTTPS TLS: " << ex.what();
            app().addListener("0.0.0.0", 8443, false);
        }
    } else {
        LOG_WARN << "GateServerDrogon: TLS cert generation failed, running in HTTP mode";
        app().addListener("0.0.0.0", 8443);
    }
}

void DrogonAppCfg::ConfigureFromConfig(const std::string& host, int port, int threads)
{
    app().setThreadNum(threads);
    app().setUploadPath("./uploads");
    app().setLogLevel(trantor::Logger::LogLevel::kInfo);
    app().setMaxConnectionNum(100000);

    std::string crt_path, key_path;
    if (EnsureSelfSignedCert(crt_path, key_path)) {
        try {
            app().setSSLFiles(crt_path, key_path);
            app().addListener(host, port, true);
            LOG_INFO << "GateServerDrogon: HTTPS/HTTP2 enabled on " << host << ":" << port;
        } catch (const std::exception& ex) {
            LOG_ERROR << "GateServerDrogon: failed to enable HTTPS TLS: " << ex.what();
            app().addListener(host, port, false);
        }
    } else {
        LOG_WARN << "GateServerDrogon: TLS cert generation failed, running in HTTP mode";
        app().addListener(host, port, false);
    }
}
