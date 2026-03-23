#pragma once
#ifndef MEMOCHAT_LOADTEST_SCENARIO_H
#define MEMOCHAT_LOADTEST_SCENARIO_H

#include "config.hpp"
#include "metrics.hpp"
#include "logger.hpp"

#include <string>
#include <memory>

namespace memochat {
namespace loadtest {

// Forward declare JsonLogger (avoid pulling in spdlog headers)
class JsonLogger;

// ---- Scenario runner interface (Strategy pattern) ----
// Each protocol scenario implements this interface.
// Isolates protocol-specific logic from the loadtest orchestration.
class IScenarioRunner {
public:
    virtual ~IScenarioRunner() = default;

    // Returns the scenario name, e.g. "tcp", "quic", "http"
    virtual std::string name() const = 0;

    // Run one iteration for a single account.
    // The caller provides a fresh trace_id.
    virtual TestResult run_one(const TestConfig& cfg,
                               const Account& account,
                               const std::string& trace_id) = 0;

    // Optional warmup hook (called before main run)
    virtual void warmup(const TestConfig& cfg,
                        const Account& account) {}

    // Called once after all workers finish
    virtual void cleanup() {}
};

// ---- Concrete scenario implementations ----

// HTTP-only login via HTTP/1.1 (plaintext, port 8080)
std::unique_ptr<IScenarioRunner> make_http_scenario();

// HTTP-only login via HTTP/2 (TLS + ALPN, port 8443)
std::unique_ptr<IScenarioRunner> make_http2_scenario();

// HTTP-only login via HTTP/3 (raw QUIC, port 8081)
// Tests QUIC handshake + HTTP/3 framing to GateServer HTTP/3 listener
std::unique_ptr<IScenarioRunner> make_http3_scenario();

// TCP login with persistent socket per iteration
std::unique_ptr<IScenarioRunner> make_tcp_scenario();

// QUIC login using msquic
std::unique_ptr<IScenarioRunner> make_quic_scenario();

// ---- Scenario factory ----
std::unique_ptr<IScenarioRunner> make_scenario(const std::string& scenario_name);

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_SCENARIO_H
