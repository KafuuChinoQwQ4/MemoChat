#include "scenario.hpp"
#include "client.hpp"

#include <chrono>

namespace memochat {
namespace loadtest {

// Resolves QUIC host/port from gate response.
// Tries chat_endpoints[transport=quic] first, then falls back to TCP+100.
static void resolve_quic_endpoint(
    const std::unordered_map<std::string, std::string>& rsp,
    std::string& out_host, int& out_port) {

    // 1. Try explicit QUIC endpoint from gate
    auto it_h = rsp.find("quic_host");
    auto it_p = rsp.find("quic_port");
    if (it_h != rsp.end() && it_p != rsp.end()) {
        out_host = it_h->second;
        try { out_port = std::stoi(it_p->second); } catch (...) {}
    }

    // 2. Fallback: TCP port + 100, host as-is
    if (out_port <= 0) {
        try {
            int tcp_port = std::stoi(rsp.at("port"));
            if (tcp_port > 0) {
                out_host = rsp.at("host");
                if (out_host.empty() || out_host == "0.0.0.0"
                    || out_host == "::" || out_host == "::1") {
                    out_host = "127.0.0.1";
                }
                out_port = tcp_port + 100;
            }
        } catch (...) {}
    }
}

class QuicScenario final : public IScenarioRunner {
public:
    explicit QuicScenario(double quic_timeout_sec = 5.0)
        : quic_timeout_sec_(quic_timeout_sec) {}

    std::string name() const override { return "quic"; }

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

        // Resolve QUIC endpoint
        std::string quic_host;
        int quic_port = 0;
        resolve_quic_endpoint(*login_rsp, quic_host, quic_port);

        if (quic_port <= 0) {
            result.stage = "quic_port_missing";
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            return result;
        }

        std::string server_name = login_rsp->at("server_name");
        if (server_name.empty()) server_name = "chatserver";

        // Phase 2: QUIC connect
        auto t_quic = std::chrono::steady_clock::now();

        QuicChatClient client(quic_host, quic_port, server_name,
                              cfg.quic_alpn, quic_timeout_sec_);

        if (!client.connect()) {
            result.stage = "quic_connect_failed:" + client.last_error();
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            client.close();
            return result;
        }

        result.quic_connect_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_quic).count();

        // Phase 3: QUIC chat login
        auto t_chat = std::chrono::steady_clock::now();

        if (!client.send_chat_login(*login_rsp, result.trace_id, cfg.protocol_version)) {
            result.stage = "quic_send_failed:" + client.last_error();
            result.elapsed_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            client.close();
            return result;
        }

        int chat_err = client.wait_chat_login_response(quic_timeout_sec_);

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
    double quic_timeout_sec_ = 5.0;
};

std::unique_ptr<IScenarioRunner> make_quic_scenario() {
    return std::make_unique<QuicScenario>();
}

// ---- Scenario factory ----
std::unique_ptr<IScenarioRunner> make_scenario(const std::string& scenario_name) {
    if (scenario_name == "tcp") return make_tcp_scenario();
    if (scenario_name == "quic") return make_quic_scenario();
    if (scenario_name == "http") return make_http_scenario();
    // Default to TCP
    return make_tcp_scenario();
}

} // namespace loadtest
} // namespace memochat
