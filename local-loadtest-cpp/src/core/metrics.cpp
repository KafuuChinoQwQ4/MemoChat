#include "metrics.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <numeric>

namespace memochat {
namespace loadtest {

// ---- MetricsCollector ----
MetricsCollector& MetricsCollector::instance() {
    static MetricsCollector inst;
    return inst;
}

void MetricsCollector::reset() {
    std::unique_lock<std::shared_mutex> lock(mu_);
    total_.store(0);
    success_.store(0);
    failed_.store(0);
    results_.clear();
    all_latencies_.clear();
    http_latencies_.clear();
    tcp_handshake_latencies_.clear();
    quic_connect_latencies_.clear();
    chat_login_latencies_.clear();
    error_counter_.clear();
}

void MetricsCollector::set_start_time() {
    start_time_ = std::chrono::steady_clock::now();
}

void MetricsCollector::set_end_time() {
    end_time_ = std::chrono::steady_clock::now();
}

void MetricsCollector::record(const TestResult& result) {
    std::unique_lock<std::shared_mutex> lock(mu_);

    total_.fetch_add(1);
    if (result.ok) {
        success_.fetch_add(1);
    } else {
        failed_.fetch_add(1);
        error_counter_[result.stage]++;
    }

    results_.push_back(result);
    all_latencies_.push_back(result.elapsed_ms);

    if (result.http_ms > 0)
        http_latencies_.push_back(result.http_ms);
    if (result.tcp_handshake_ms > 0)
        tcp_handshake_latencies_.push_back(result.tcp_handshake_ms);
    if (result.quic_connect_ms > 0)
        quic_connect_latencies_.push_back(result.quic_connect_ms);
    if (result.chat_login_ms > 0)
        chat_login_latencies_.push_back(result.chat_login_ms);
}

double MetricsCollector::percentile(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) return 0.0;
    size_t n = sorted.size();
    double idx = std::ceil(n * p) - 1;
    idx = std::max(double(0), std::min(idx, double(n - 1)));
    return sorted[static_cast<size_t>(idx)];
}

LatencyStats MetricsCollector::compute_stats(const std::vector<double>& latencies) {
    LatencyStats s;
    if (latencies.empty()) return s;

    std::vector<double> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());
    s.min_ms = sorted.front();
    s.max_ms = sorted.back();
    s.avg_ms = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
    s.p50_ms  = percentile(sorted, 0.50);
    s.p75_ms  = percentile(sorted, 0.75);
    s.p90_ms  = percentile(sorted, 0.90);
    s.p95_ms  = percentile(sorted, 0.95);
    s.p98_ms  = percentile(sorted, 0.98);
    s.p99_ms  = percentile(sorted, 0.99);
    s.p999_ms = percentile(sorted, 0.999);
    return s;
}

TestSummary MetricsCollector::build_summary() const {
    std::shared_lock<std::shared_mutex> lock(mu_);

    TestSummary summary;
    summary.total = total_.load();
    summary.success = success_.load();
    summary.failed = failed_.load();

    double elapsed = std::chrono::duration<double>(end_time_ - start_time_).count();
    summary.elapsed_sec = elapsed;
    summary.rps = (elapsed > 0) ? (summary.total / elapsed) : 0;
    summary.success_rate = (summary.total > 0) ? (double(summary.success) / summary.total) : 0;

    summary.latency_ms = compute_stats(all_latencies_);
    summary.http_phase = compute_stats(http_latencies_);
    summary.tcp_handshake_phase = compute_stats(tcp_handshake_latencies_);
    summary.quic_connect_phase = compute_stats(quic_connect_latencies_);
    summary.chat_login_phase = compute_stats(chat_login_latencies_);

    // Top 10 errors
    std::vector<std::pair<std::string, int>> sorted_errors(
        error_counter_.begin(), error_counter_.end());
    std::sort(sorted_errors.begin(), sorted_errors.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    summary.error_counter = error_counter_;
    size_t top_n = std::min(sorted_errors.size(), size_t(10));
    summary.top_errors.assign(sorted_errors.begin(), sorted_errors.begin() + top_n);

    return summary;
}

// ---- Report JSON output ----
void save_report_json(const std::string& path,
                      const TestSummary& summary,
                      const std::string& scenario,
                      const std::unordered_map<std::string, std::string>& extra) {
    std::ofstream f(path);
    if (!f) return;

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc = *std::gmtime(&now_c);
    char ts[64];
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_utc);

    auto stat_to_json = [&](const LatencyStats& s) {
        std::ostringstream ss;
        ss << R"({"min":)" << s.min_ms
           << R"(,"avg":)" << s.avg_ms
           << R"(,"p50":)" << s.p50_ms
           << R"(,"p75":)" << s.p75_ms
           << R"(,"p90":)" << s.p90_ms
           << R"(,"p95":)" << s.p95_ms
           << R"(,"p98":)" << s.p98_ms
           << R"(,"p99":)" << s.p99_ms
           << R"(,"p999":)" << s.p999_ms
           << R"(,"max":)" << s.max_ms << "}";
        return ss.str();
    };

    f << "{\n";
    f << "  \"scenario\": \"" << scenario << "\",\n";
    f << "  \"time_utc\": \"" << ts << "\",\n";
    f << "  \"total\": " << summary.total << ",\n";
    f << "  \"success\": " << summary.success << ",\n";
    f << "  \"failed\": " << summary.failed << ",\n";
    f << "  \"success_rate\": " << std::fixed << std::setprecision(6) << summary.success_rate << ",\n";
    f << "  \"elapsed_sec\": " << std::fixed << std::setprecision(6) << summary.elapsed_sec << ",\n";
    f << "  \"rps\": " << std::fixed << std::setprecision(3) << summary.rps << ",\n";
    f << "  \"latency_ms\": " << stat_to_json(summary.latency_ms) << ",\n";
    f << "  \"phase_breakdown\": {\n";

    bool first = true;
    auto phase_json = [&](const std::string& name, const LatencyStats& s) {
        if (!first) f << ",\n";
        first = false;
        f << "    \"" << name << "\": " << stat_to_json(s);
    };

    phase_json("http_gate", summary.http_phase);
    phase_json("tcp_handshake", summary.tcp_handshake_phase);
    phase_json("quic_connect", summary.quic_connect_phase);
    phase_json("chat_login", summary.chat_login_phase);

    f << "\n  },\n";

    // error_counter
    f << "  \"error_counter\": {";
    first = true;
    for (const auto& [k, v] : summary.error_counter) {
        if (!first) f << ",";
        first = false;
        f << "\"" << k << "\":" << v;
    }
    f << "},\n";

    // top_errors
    f << "  \"top_errors\": [";
    first = true;
    for (const auto& [k, v] : summary.top_errors) {
        if (!first) f << ",";
        first = false;
        f << "[\"" << k << "\"," << v << "]";
    }
    f << "],\n";

    // extra attrs
    f << "  \"extra\": {";
    first = true;
    for (const auto& [k, v] : extra) {
        if (!first) f << ",";
        first = false;
        f << "\"" << k << "\":\"" << v << "\"";
    }
    f << "}\n";

    f << "}\n";
    f.close();
}

} // namespace loadtest
} // namespace memochat
