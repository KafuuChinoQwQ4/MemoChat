#pragma once

#include <boost/asio.hpp>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace memochat::runtime {

class IoContextPool {
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::executor_work_guard<IOService::executor_type>;
    using WorkPtr = std::unique_ptr<Work>;

    explicit IoContextPool(std::size_t size);
    ~IoContextPool();

    IoContextPool(const IoContextPool&) = delete;
    IoContextPool& operator=(const IoContextPool&) = delete;

    boost::asio::io_context& GetIOService();
    void Stop();

private:
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t _nextIOService = 0;
    bool _stopped = false;
};

} // namespace memochat::runtime
