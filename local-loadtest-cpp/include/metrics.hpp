#pragma once
#ifndef MEMOCHAT_LOADTEST_METRICS_H
#define MEMOCHAT_LOADTEST_METRICS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <cstdint>
#include <optional>
#include <chrono>

namespace memochat {
namespace loadtest {

struct TestResult {
    bool ok = false;
    std::string trace_id;
    std::string stage;
    double elapsed_ms = 0.0;

    // Phase timings
    double http_ms = 0.0;
    double tcp_handshake_ms = 0.0;
    double quic_connect_ms = 0.0;
    double chat_login_ms = 0.0;
};

struct LatencyStats {
    double min_ms = 0.0;
    double avg_ms = 0.0;
    double p50_ms = 0.0;
    double p75_ms = 0.0;
    double p90_ms = 0.0;
    double p95_ms = 0.0;
    double p98_ms = 0.0;
    double p99_ms = 0.0;
    double p999_ms = 0.0;
    double max_ms = 0.0;
};

struct TestSummary {
    int total = 0;
    int success = 0;
    int failed = 0;
    double success_rate = 0.0;
    double elapsed_sec = 0.0;
    double rps = 0.0;
    LatencyStats latency_ms;

    // Phase breakdowns
    LatencyStats http_phase;
    LatencyStats tcp_handshake_phase;
    LatencyStats quic_connect_phase;
    LatencyStats chat_login_phase;

    // Error distribution
    std::unordered_map<std::string, int> error_counter;

    // Top errors
    std::vector<std::pair<std::string, int>> top_errors;
};

class MetricsCollector {
public:
    static MetricsCollector& instance();

    void reset();

    void record(const TestResult& result);

    void set_start_time();
    void set_end_time();

    TestSummary build_summary() const;

    int total() const { return total_.load(); }
    int success() const { return success_.load(); }
    int failed() const { return failed_.load(); }

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    static double percentile(const std::vector<double>& sorted, double p);
    static LatencyStats compute_stats(const std::vector<double>& latencies);

    mutable std::shared_mutex mu_;

    std::atomic<int> total_{0};
    std::atomic<int> success_{0};
    std::atomic<int> failed_{0};

    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point end_time_;

    std::vector<TestResult> results_;
    std::vector<double> all_latencies_;
    std::vector<double> http_latencies_;
    std::vector<double> tcp_handshake_latencies_;
    std::vector<double> quic_connect_latencies_;
    std::vector<double> chat_login_latencies_;

    std::unordered_map<std::string, int> error_counter_;
};

// Report output
void save_report_json(const std::string& path,
                      const TestSummary& summary,
                      const std::string& scenario,
                      const std::unordered_map<std::string, std::string>& extra = {});

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_METRICS_H
