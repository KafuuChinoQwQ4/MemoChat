#include "runner.hpp"
#include "client.hpp"

#include <algorithm>
#include <thread>
#include <atomic>
#include <future>
#include <queue>

namespace memochat {
namespace loadtest {

// ---- Shared core logic ----
static TestSummary run_loadtest_impl(const TestConfig& cfg,
                                      const RunnerConfig& runner_cfg,
                                      JsonLogger& logger,
                                      std::vector<Account> accounts) {
    MetricsCollector::instance().reset();
    MetricsCollector::instance().set_start_time();

    // Progress monitor
    ProgressMonitor& monitor = ProgressMonitor::instance();
    monitor.start(runner_cfg.total, runner_cfg.concurrency, runner_cfg.scenario);

    if (accounts.empty()) {
        logger.error("No accounts available for load test");
        return MetricsCollector::instance().build_summary();
    }

    logger.info("Running with " + std::to_string(accounts.size()) + " accounts, total="
                + std::to_string(runner_cfg.total) + " requests");

    // Build scenario via factory (decoupled strategy pattern)
    auto scenario = make_scenario(runner_cfg.scenario);

    // Optional warmup
    if (runner_cfg.warmup_iters > 0) {
        logger.info("Starting warmup: " + std::to_string(runner_cfg.warmup_iters) + " iterations");
        for (int w = 0; w < runner_cfg.warmup_iters; ++w) {
            const Account& acc = accounts[w % accounts.size()];
            scenario->warmup(cfg, acc);
        }
        logger.info("Warmup completed");
    }

    logger.info("Starting load test: total=" + std::to_string(runner_cfg.total)
                + " concurrency=" + std::to_string(runner_cfg.concurrency)
                + " scenario=" + scenario->name());

    // Fixed-size thread pool to bound concurrency.
    // Treat concurrency=0 as "use default concurrency" (50), not "0 threads".
    const int num_threads = runner_cfg.concurrency > 0 ? runner_cfg.concurrency : 50;
    std::atomic<int> done{0};
    std::mutex result_mu;

    // Thread pool workers drain a shared task queue
    std::mutex queue_mu;
    std::condition_variable cv;
    std::queue<std::packaged_task<TestResult()>> task_queue;
    std::atomic<bool> all_submitted{false};

    auto get_task = [&]() -> std::packaged_task<TestResult()> {
        std::unique_lock<std::mutex> lk(queue_mu);
        cv.wait(lk, [&] { return !task_queue.empty() || all_submitted.load(); });
        if (task_queue.empty()) return {};
        auto task = std::move(task_queue.front());
        task_queue.pop();
        return task;
    };

    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            while (true) {
                auto task = get_task();
                if (!task.valid()) break; // queue empty and all submitted
                TestResult r = task.get_future().get();
                {
                    std::lock_guard<std::mutex> lk2(result_mu);
                    MetricsCollector::instance().record(r);
                    int d = ++done;

                    if (!r.ok) {
                        logger.warn("Request failed: trace_id=" + r.trace_id + " stage=" + r.stage);
                    }

                    monitor.update(d,
                        MetricsCollector::instance().success(),
                        MetricsCollector::instance().failed());
                }
            }
        });
    }

    // Submit all tasks (captures by value via copies in packaged_task)
    for (int i = 0; i < runner_cfg.total; ++i) {
        const Account& acc = accounts[i % accounts.size()];
        std::string trace_id = make_trace_id();

        std::packaged_task<TestResult()> task([&, acc, trace_id]() {
            return scenario->run_one(cfg, acc, trace_id);
        });

        {
            std::lock_guard<std::mutex> lk(queue_mu);
            task_queue.push(std::move(task));
        }
        cv.notify_one();
    }

    {
        std::lock_guard<std::mutex> lk(queue_mu);
        all_submitted = true;
    }
    cv.notify_all();

    // Wait for all workers to drain
    for (auto& w : workers) w.join();

    MetricsCollector::instance().set_end_time();
    monitor.stop();

    // Cleanup scenario resources (e.g. QUIC library)
    scenario->cleanup();

    return MetricsCollector::instance().build_summary();
}

// ---- Main runner: loads accounts from CSV ----
TestSummary run_loadtest(const TestConfig& cfg,
                          const RunnerConfig& runner_cfg,
                          JsonLogger& logger) {
    std::vector<Account> accounts = load_accounts_csv(cfg.accounts_csv);
    if (accounts.empty()) {
        logger.error("No accounts loaded from: " + cfg.accounts_csv);
        return MetricsCollector::instance().build_summary();
    }
    logger.info("Loaded " + std::to_string(accounts.size()) + " accounts from CSV");
    return run_loadtest_impl(cfg, runner_cfg, logger, std::move(accounts));
}

// ---- Runner with pre-filtered accounts ----
TestSummary run_loadtest(const TestConfig& cfg,
                          const RunnerConfig& runner_cfg,
                          JsonLogger& logger,
                          std::vector<Account> accounts) {
    return run_loadtest_impl(cfg, runner_cfg, logger, std::move(accounts));
}

} // namespace loadtest
} // namespace memochat
