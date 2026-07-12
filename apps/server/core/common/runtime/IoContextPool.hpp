#pragma once

#include <boost/asio.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "runtime/ExplicitThread.hpp"

namespace memochat::runtime
{

class IoContextPool
{
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
    bool Ready() const noexcept;
    const std::string& startupError() const noexcept;

private:
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<ExplicitThread> _threads;
    std::size_t _nextIOService = 0;
    bool _stopped = false;
    std::string _startup_error;
};

} // namespace memochat::runtime
