#include "scenario.hpp"
#include "client.hpp"

namespace memochat {
namespace loadtest {

// Resolves host/port from gate login response with localhost fallbacks.
static void resolve_chat_endpoint(
    const std::unordered_map<std::string, std::string>& rsp,
    std::string& out_host, int& out_port) {
    out_host = rsp.at("host");
    if (out_host.empty() || out_host == "0.0.0.0" || out_host == "::" || out_host == "::1") {
        out_host = "127.0.0.1";
    }
    try { out_port = std::stoi(rsp.at("port")); } catch (...) { out_port = 0; }
}

class TcpScenario final : public IScenarioRunner {
public:
    explicit TcpScenario(int max_retries = 2) : max_retries_(max_retries) {}

    std::string name() const override { return "tcp"; }

    TestResult run_one(const TestConfig& cfg,
                       const Account& account,
                       const std::string& trace_id) override {
        TestResult result;
        result.trace_id = trace_id;

        auto t0 = std::chrono::steady_clock::now();

        // Phase 1: HTTP Gate login
        auto t_http = std::chrono::steady_clock::now();
        auto login_rsp = gate_login(cfg, account, result.trace_id);
        result.http_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_http).count();

        if (!login_rsp) {
            result.stage = "gate_login_failed";
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            return result;
        }

        int uid = 0;
        try { uid = std::stoi(login_rsp->at("uid")); } catch (...) {}
        if (uid <= 0) {
            result.stage = "invalid_uid";
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            return result;
        }

        std::string host;
        int port = 0;
        resolve_chat_endpoint(*login_rsp, host, port);

        if (port <= 0) {
            result.stage = "invalid_port";
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            return result;
        }

        // Phase 2: TCP handshake + chat login
        auto t_tcp = std::chrono::steady_clock::now();

        TcpChatClient client(cfg.tcp_timeout_sec);
        if (!client.connect(host, port)) {
            result.stage = "tcp_connect_failed:" + client.last_error();
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            return result;
        }

        result.tcp_handshake_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_tcp).count();

        auto t_chat = std::chrono::steady_clock::now();
        int chat_err = client.chat_login(*login_rsp, result.trace_id,
                                         cfg.protocol_version, cfg.tcp_timeout_sec);
        result.chat_login_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_chat).count();

        client.close();

        result.elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();

        if (chat_err == 0) {
            result.ok = true;
            result.stage = "ok";
        } else {
            result.stage = "chat_login_error_" + std::to_string(chat_err);
        }

        return result;
    }

    void warmup(const TestConfig& cfg, const Account& account) override {
        TestResult r = run_one(cfg, account, "warmup");
        (void)r;
    }

private:
    int max_retries_ = 2;
};

std::unique_ptr<IScenarioRunner> make_tcp_scenario() {
    return std::make_unique<TcpScenario>();
}

} // namespace loadtest
} // namespace memochat
