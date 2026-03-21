#include "runner.hpp"
#include "logger.hpp"
#include "metrics.hpp"
#include "config.hpp"
#include "client.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace fs = std::filesystem;
using namespace memochat::loadtest;

static void print_banner() {
    std::cout << "\n============================================================\n";
    std::cout << "  MemoChat Load Test Framework  (C++ / msquic)\n";
    std::cout << "============================================================\n\n";
}

static void print_summary(const TestSummary& s, const std::string& scenario) {
    std::cout << "\n============================================================\n";
    std::cout << "  " << scenario << " Load Test Results\n";
    std::cout << "============================================================\n";
    std::cout << "  Total:     " << s.total
              << "   Success: " << s.success
              << "   Failed: " << s.failed << "\n";
    std::cout << "  RPS:       " << std::fixed << std::setprecision(2) << s.rps << "\n";
    std::cout << "  Avg:       " << s.latency_ms.avg_ms << " ms\n";
    std::cout << "  P50:       " << s.latency_ms.p50_ms << " ms\n";
    std::cout << "  P75:       " << s.latency_ms.p75_ms << " ms\n";
    std::cout << "  P90:       " << s.latency_ms.p90_ms << " ms\n";
    std::cout << "  P95:       " << s.latency_ms.p95_ms << " ms\n";
    std::cout << "  P98:       " << s.latency_ms.p98_ms << " ms\n";
    std::cout << "  P99:       " << s.latency_ms.p99_ms << " ms\n";
    std::cout << "  P999:      " << s.latency_ms.p999_ms << " ms\n";
    std::cout << "  Max:       " << s.latency_ms.max_ms << " ms\n";
    std::cout << "  Success:   " << std::fixed << std::setprecision(2)
              << (s.success_rate * 100) << "%\n";

    if (!s.top_errors.empty()) {
        std::cout << "\n  Top Errors:\n";
        for (const auto& [err, count] : s.top_errors) {
            std::cout << "    " << err << ": " << count << "\n";
        }
    }

    // Phase breakdown
    if (s.http_phase.avg_ms > 0 || s.tcp_handshake_phase.avg_ms > 0
        || s.quic_connect_phase.avg_ms > 0 || s.chat_login_phase.avg_ms > 0) {
        std::cout << "\n  Phase Breakdown:\n";
        if (s.http_phase.avg_ms > 0)
            std::cout << "    http_gate:     avg=" << s.http_phase.avg_ms << " ms"
                      << "  p95=" << s.http_phase.p95_ms << " ms\n";
        if (s.tcp_handshake_phase.avg_ms > 0)
            std::cout << "    tcp_handshake: avg=" << s.tcp_handshake_phase.avg_ms << " ms"
                      << "  p95=" << s.tcp_handshake_phase.p95_ms << " ms\n";
        if (s.quic_connect_phase.avg_ms > 0)
            std::cout << "    quic_connect:  avg=" << s.quic_connect_phase.avg_ms << " ms"
                      << "  p95=" << s.quic_connect_phase.p95_ms << " ms\n";
        if (s.chat_login_phase.avg_ms > 0)
            std::cout << "    chat_login:    avg=" << s.chat_login_phase.avg_ms << " ms"
                      << "  p95=" << s.chat_login_phase.p95_ms << " ms\n";
    }

    std::cout << "============================================================\n";
}

static std::string make_report_name(const std::string& scenario,
                                    const std::string& report_dir) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc = *std::gmtime(&t);

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_utc);

    fs::create_directories(report_dir);
    return (fs::path(report_dir) / (scenario + "_" + buf + ".json")).string();
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    print_banner();

    // Default paths
    std::string cfg_path = "config.json";
    std::string scenario = "tcp";
    bool filter_accounts = false;

    // Parse CLI args
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            cfg_path = argv[++i];
        } else if (arg == "--scenario" && i + 1 < argc) {
            scenario = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "  --config <path>       Config file (default: config.json)\n"
                      << "  --scenario <name>     tcp|quic|http (default: tcp)\n"
                      << "  --total <n>          Total requests\n"
                      << "  --concurrency <n>     Concurrent workers\n"
                      << "  --warmup <n>         Warmup iterations\n"
                      << "  --filter-accounts     Probe each account via HTTP and keep only valid ones\n";
            return 0;
        }
    }

    // Load config
    TestConfig cfg;
    if (!load_config(cfg_path, cfg)) {
        std::cerr << "ERROR: Failed to load config from: " << cfg_path << "\n";
        return 1;
    }

    // Override scenario-specific params
    RunnerConfig runner;
    runner.scenario = scenario;
    runner.total = cfg.total;
    runner.concurrency = cfg.concurrency;
    runner.warmup_iters = cfg.warmup_iterations;
    runner.max_retries = cfg.max_retries;

    // Override with CLI args
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--total" && i + 1 < argc) {
            runner.total = std::atoi(argv[++i]);
        } else if (arg == "--concurrency" && i + 1 < argc) {
            runner.concurrency = std::atoi(argv[++i]);
        } else if (arg == "--warmup" && i + 1 < argc) {
            runner.warmup_iters = std::atoi(argv[++i]);
        } else if (arg == "--retries" && i + 1 < argc) {
            runner.max_retries = std::atoi(argv[++i]);
        } else if (arg == "--filter-accounts") {
            filter_accounts = true;
        }
    }

    // Init logger
    JsonLogger& logger = JsonLogger::instance();
    logger.init("loadtest_" + scenario, cfg.log_dir, LogLevel::LV_INFO);

    logger.info("Starting " + scenario + " loadtest: config=" + cfg_path
                + " total=" + std::to_string(runner.total)
                + " concurrency=" + std::to_string(runner.concurrency));

    // Load accounts (all CLI args have been parsed)
    std::vector<Account> accounts = load_accounts_csv(cfg.accounts_csv);
    if (accounts.empty()) {
        logger.error("No accounts loaded from: " + cfg.accounts_csv);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // Optional: probe each account via HTTP and keep only those that return error=0
    if (filter_accounts) {
        logger.info("Probing accounts (filter mode)...");
        accounts = filter_ready_accounts(cfg, accounts, cfg.http_timeout_sec);
        if (accounts.empty()) {
            logger.error("No valid accounts after HTTP probe");
#ifdef _WIN32
            WSACleanup();
#endif
            return 1;
        }
        logger.info("Filtered to " + std::to_string(accounts.size()) + " valid accounts");
    } else {
        logger.info("Loaded " + std::to_string(accounts.size()) + " accounts");
    }

    // Adjust total if there aren't enough accounts
    if (runner.total > (int)accounts.size() * 100) {
        logger.warn("total=" + std::to_string(runner.total)
                    + " exceeds 100x account count ("
                    + std::to_string(accounts.size())
                    + "); threads will reuse accounts");
    }

    // Run (use pre-loaded accounts)
    TestSummary summary = run_loadtest(cfg, runner, logger, std::move(accounts));

    // Report
    std::string report_path = make_report_name(scenario + "_login", cfg.report_dir);
    std::unordered_map<std::string, std::string> extra;
    extra["scenario"] = "login." + scenario;
    extra["config_path"] = cfg_path;

    save_report_json(report_path, summary, "login." + scenario, extra);

    logger.info("Report saved: " + report_path);

    print_summary(summary, scenario);

    std::cout << "\nReport: " << report_path << "\n";

#ifdef _WIN32
    WSACleanup();
#endif

    return (summary.failed == 0) ? 0 : 1;
}
