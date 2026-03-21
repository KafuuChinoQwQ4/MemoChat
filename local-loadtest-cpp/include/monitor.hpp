#pragma once
#ifndef MEMOCHAT_LOADTEST_MONITOR_H
#define MEMOCHAT_LOADTEST_MONITOR_H

#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <optional>

namespace memochat {
namespace loadtest {

// Memory / CPU usage monitor
// Must be defined before ProgressMonitor (referenced as a data member type)
struct SystemMetrics {
    double cpu_percent = 0.0;
    size_t mem_used_mb = 0;
    size_t mem_total_mb = 0;
    double mem_percent = 0.0;
};

// Real-time progress monitor with terminal output
class ProgressMonitor {
public:
    static ProgressMonitor& instance();

    void start(int total, int concurrency, const std::string& scenario);
    void stop();

    void update(int completed, int success, int failed);

    // Snapshot for external readers
    struct Snapshot {
        int completed = 0;
        int total = 0;
        int success = 0;
        int failed = 0;
        double elapsed_sec = 0.0;
        double current_rps = 0.0;
        double instant_rps = 0.0;
        double eta_sec = 0.0;
        double p50_ms = 0.0;
        double p95_ms = 0.0;
    };

    Snapshot snapshot() const;

    // Progress bar callback type
    using ProgressCallback = std::function<void(const Snapshot&)>;

    void set_callback(ProgressCallback cb);

    // System metrics snapshot (last heartbeat sample)
    SystemMetrics last_system_metrics() const;

private:
    ProgressMonitor() = default;
    ~ProgressMonitor();

    ProgressMonitor(const ProgressMonitor&) = delete;
    ProgressMonitor& operator=(const ProgressMonitor&) = delete;

    void run_heartbeat();

    mutable std::shared_mutex mu_;
    int total_ = 0;
    int concurrency_ = 0;
    std::string scenario_;

    std::atomic<int> completed_{0};
    std::atomic<int> success_{0};
    std::atomic<int> failed_{0};

    std::chrono::steady_clock::time_point start_time_;

    bool running_ = false;
    std::thread heartbeat_thread_;
    ProgressCallback callback_;

    // Rolling window for instant RPS
    static constexpr size_t RPS_WINDOW_SIZE = 10;
    std::vector<std::pair<std::chrono::steady_clock::time_point, int>> rps_window_;
    mutable std::mutex rps_mu_;

    // System metrics (sampled on heartbeat thread)
    SystemMetrics last_sys_;
    mutable std::mutex sys_mu_;
};

// SystemMetrics is now defined before ProgressMonitor (see above)

SystemMetrics get_system_metrics();

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_MONITOR_H
