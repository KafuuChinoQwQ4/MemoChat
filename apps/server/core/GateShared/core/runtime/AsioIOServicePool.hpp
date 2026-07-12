#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include "Singleton.hpp"
#include "runtime/IoContextPool.hpp"

class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;

public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::executor_work_guard<IOService::executor_type>;
    using WorkPtr = std::unique_ptr<Work>;

    explicit AsioIOServicePool(std::size_t size = 1);
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;

    boost::asio::io_context& GetIOService();
    void Stop();
    bool Ready() const noexcept;
    const std::string& startupError() const noexcept;

private:
    std::unique_ptr<memochat::runtime::IoContextPool> _pool;
};
