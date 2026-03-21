#pragma once
#ifndef MEMOCHAT_LOADTEST_RUNNER_H
#define MEMOCHAT_LOADTEST_RUNNER_H

#include "scenario.hpp"
#include "metrics.hpp"
#include "monitor.hpp"
#include "logger.hpp"
#include "config.hpp"

namespace memochat {
namespace loadtest {

struct RunnerConfig {
    std::string scenario;   // "tcp", "quic", "http"
    int total = 500;
    int concurrency = 50;
    int warmup_iters = 2;
    int max_retries = 2;
};

// Main orchestration entry point.
// Loads accounts, runs warmup, dispatches async workers via the scenario strategy,
// collects metrics, and returns the final summary.
TestSummary run_loadtest(const TestConfig& cfg,
                          const RunnerConfig& runner_cfg,
                          JsonLogger& logger);

// Overload: use pre-filtered accounts instead of loading from CSV.
TestSummary run_loadtest(const TestConfig& cfg,
                          const RunnerConfig& runner_cfg,
                          JsonLogger& logger,
                          std::vector<Account> accounts);

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_RUNNER_H
