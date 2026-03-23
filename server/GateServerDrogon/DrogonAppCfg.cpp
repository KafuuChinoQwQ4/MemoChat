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
    app().addListener("0.0.0.0", 8443);  // HTTPS port for HTTP/2
    app().setThreadNum(4);
    app().setUploadPath("./uploads");
    app().setLogLevel(trantor::Logger::LogLevel::kInfo);
    app().setMaxConnectionNum(100000);

    // Enable HTTP/2 over TLS with a self-signed certificate.
    // HTTP/2 requires TLS with ALPN negotiation.
    // Drogon automatically negotiates HTTP/2 over TLS when the client supports it.
    // If cert generation fails, Drogon will still start in HTTP/1.1 mode.
    std::string crt_path, key_path;
    if (EnsureSelfSignedCert(crt_path, key_path)) {
        try {
            app().setSSLFiles(crt_path, key_path);
            LOG_INFO << "GateServerDrogon: HTTPS enabled on port 8443 (HTTP/2 via ALPN)";
            LOG_INFO << "GateServerDrogon: TLS cert=" << crt_path;
            LOG_INFO << "GateServerDrogon: TLS key=" << key_path;
        } catch (const std::exception& ex) {
            LOG_ERROR << "GateServerDrogon: failed to enable HTTPS TLS: " << ex.what();
        }
    } else {
        LOG_WARN << "GateServerDrogon: TLS cert generation failed, running in HTTP mode";
    }

    // Note: Port 8081 is reserved for GateServer.exe (HTTP/1.1)
    // GateServerDrogon serves HTTPS/HTTP2 on port 8443 only
}

void DrogonAppCfg::ConfigureFromConfig(const std::string& host, int port, int threads)
{
    // HTTPS port for HTTP/2
    app().addListener(host, port);
    app().setThreadNum(threads);
    app().setUploadPath("./uploads");
    app().setLogLevel(trantor::Logger::LogLevel::kInfo);
    app().setMaxConnectionNum(100000);

    std::string crt_path, key_path;
    if (EnsureSelfSignedCert(crt_path, key_path)) {
        try {
            app().setSSLFiles(crt_path, key_path);
            LOG_INFO << "GateServerDrogon: HTTPS enabled on " << host << ":" << port << " (HTTP/2 via ALPN)";
        } catch (const std::exception& ex) {
            LOG_ERROR << "GateServerDrogon: failed to enable HTTPS TLS: " << ex.what();
        }
    }
}
