#include "scenario.hpp"
#include "client.hpp"

namespace memochat {
namespace loadtest {

class HttpScenario final : public IScenarioRunner {
public:
    std::string name() const override { return "http"; }

    TestResult run_one(const TestConfig& cfg,
                       const Account& account,
                       const std::string& trace_id) override {
        TestResult result;
        result.trace_id = trace_id;

        auto t0 = std::chrono::steady_clock::now();
        auto login_rsp = gate_login(cfg, account, result.trace_id);
        result.elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();

        if (login_rsp) {
            try {
                int err = std::stoi(login_rsp->at("error"));
                result.ok = (err == 0);
                result.stage = result.ok ? "ok" : "error_" + std::to_string(err);
            } catch (...) {
                result.stage = "parse_error";
            }
        } else {
            result.stage = "gate_login_failed";
        }

        return result;
    }

    void warmup(const TestConfig& cfg, const Account& account) override {
        gate_login(cfg, account, "warmup");
    }
};

std::unique_ptr<IScenarioRunner> make_http_scenario() {
    return std::make_unique<HttpScenario>();
}

} // namespace loadtest
} // namespace memochat
