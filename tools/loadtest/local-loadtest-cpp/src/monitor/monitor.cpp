#include "monitor.hpp"

#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include "../../../../../apps/server/core/common/WinSdkCompat.h"
#include <psapi.h>
#include <processthreadsapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace memochat {
namespace loadtest {

// ---- ProgressMonitor ----
ProgressMonitor& ProgressMonitor::instance() {
    static ProgressMonitor inst;
    return inst;
}

ProgressMonitor::~ProgressMonitor() {
    stop();
}

void ProgressMonitor::start(int total, int concurrency, const std::string& scenario) {
    std::unique_lock<std::shared_mutex> lock(mu_);
    total_ = total;
    concurrency_ = concurrency;
    scenario_ = scenario;
    completed_.store(0);
    success_.store(0);
    failed_.store(0);
    start_time_ = std::chrono::steady_clock::now();
    running_ = true;
    rps_window_.clear();

    heartbeat_thread_ = std::thread([this]() { run_heartbeat(); });
}

void ProgressMonitor::stop() {
    {
        std::unique_lock<std::shared_mutex> lock(mu_);
        if (!running_) return;
        running_ = false;
    }
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

void ProgressMonitor::update(int completed, int success, int failed) {
    completed_.store(completed);
    success_.store(success);
    failed_.store(failed);

    std::lock_guard<std::mutex> lock(rps_mu_);
    auto now = std::chrono::steady_clock::now();
    rps_window_.emplace_back(now, completed);

    // Keep only last RPS_WINDOW_SIZE entries
    while (rps_window_.size() > RPS_WINDOW_SIZE) {
        rps_window_.erase(rps_window_.begin());
    }
}

ProgressMonitor::Snapshot ProgressMonitor::snapshot() const {
    std::shared_lock<std::shared_mutex> lock(mu_);
    Snapshot snap;
    snap.completed = completed_.load();
    snap.total = total_;
    snap.success = success_.load();
    snap.failed = failed_.load();

    auto now = std::chrono::steady_clock::now();
    snap.elapsed_sec = std::chrono::duration<double>(now - start_time_).count();

    if (snap.elapsed_sec > 0 && snap.total > 0) {
        snap.current_rps = snap.completed / snap.elapsed_sec;
        int remaining = snap.total - snap.completed;
        snap.eta_sec = (snap.current_rps > 0) ? (remaining / snap.current_rps) : 0;
    }

    // Instant RPS from rolling window
    std::lock_guard<std::mutex> rlock(rps_mu_);
    if (rps_window_.size() >= 2) {
        const auto& first = rps_window_.front();
        const auto& last = rps_window_.back();
        double dt = std::chrono::duration<double>(last.first - first.first).count();
        if (dt > 0) {
            snap.instant_rps = (last.second - first.second) / dt;
        }
    }

    return snap;
}

void ProgressMonitor::run_heartbeat() {
    int last_completed = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        bool is_running;
        {
            std::shared_lock<std::shared_mutex> lock(mu_);
            is_running = running_;
        }
        if (!is_running) break;

        auto snap = snapshot();
        double progress_pct = (snap.total > 0) ? (100.0 * snap.completed / snap.total) : 0;

        // Sample system metrics
        {
            std::lock_guard<std::mutex> slock(sys_mu_);
            last_sys_ = get_system_metrics();
        }

        std::cout << "\r  ["
                  << std::fixed << std::setprecision(1) << progress_pct << "%"
                  << "] completed=" << snap.completed << "/" << snap.total
                  << " success=" << snap.success << " failed=" << snap.failed
                  << " rps=" << std::setprecision(1) << snap.current_rps
                  << " eta=" << std::setprecision(0) << snap.eta_sec << "s"
                  << " cpu=" << std::setprecision(1) << last_sys_.cpu_percent << "%"
                  << " rss=" << last_sys_.mem_used_mb << "MB"
                  << "                " << std::flush;

        if (callback_) {
            callback_(snap);
        }

        last_completed = snap.completed;
    }

    // Final line
    auto snap = snapshot();
    double progress_pct = (snap.total > 0) ? (100.0 * snap.completed / snap.total) : 0;
    {
        std::lock_guard<std::mutex> slock(sys_mu_);
        last_sys_ = get_system_metrics();
    }
    std::cout << "\r  ["
              << std::fixed << std::setprecision(1) << progress_pct << "%"
              << "] completed=" << snap.completed << "/" << snap.total
              << " success=" << snap.success << " failed=" << snap.failed
              << " rps=" << std::setprecision(1) << snap.current_rps
              << " cpu=" << std::setprecision(1) << last_sys_.cpu_percent << "%"
              << " rss=" << last_sys_.mem_used_mb << "MB"
              << " (done)               " << std::endl;
}

void ProgressMonitor::set_callback(ProgressCallback cb) {
    std::lock_guard<std::shared_mutex> lock(mu_);
    callback_ = std::move(cb);
}

SystemMetrics ProgressMonitor::last_system_metrics() const {
    std::lock_guard<std::mutex> slock(sys_mu_);
    return last_sys_;
}

// ---- SystemMetrics ----
SystemMetrics get_system_metrics() {
    SystemMetrics m;
#ifdef _WIN32
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        m.mem_used_mb = pmc.WorkingSetSize / (1024 * 1024);
    }

    MEMORYSTATUSEX mse;
    mse.dwLength = sizeof(mse);
    if (GlobalMemoryStatusEx(&mse)) {
        m.mem_total_mb = (size_t)(mse.ullTotalPhys / (1024 * 1024));
        if (m.mem_total_mb > 0) {
            m.mem_percent = 100.0 * m.mem_used_mb / m.mem_total_mb;
        }
    }

    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        static ULONGLONG last_idle = 0, last_total = 0;
        ULONGLONG idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
        ULONGLONG kernel_t = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
        ULONGLONG user_t = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
        ULONGLONG total = kernel_t + user_t;
        if (last_total > 0) {
            ULONGLONG d_idle = idle - last_idle;
            ULONGLONG d_total = total - last_total;
            if (d_total > 0) {
                m.cpu_percent = 100.0 * (1.0 - double(d_idle) / double(d_total));
            }
        }
        last_idle = idle;
        last_total = total;
    }
#else
    // Unix: CPU via getrusage, memory via /proc
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    m.cpu_percent = (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 100.0
                  + (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 10000.0;
    m.mem_used_mb = usage.ru_maxrss / 1024;
#endif
    return m;
}

} // namespace loadtest
} // namespace memochat
