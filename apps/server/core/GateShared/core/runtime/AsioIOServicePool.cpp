#include "AsioIOServicePool.hpp"

AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _pool(std::make_unique<memochat::runtime::IoContextPool>(size))
{
}

AsioIOServicePool::~AsioIOServicePool() = default;

boost::asio::io_context& AsioIOServicePool::GetIOService()
{
    return _pool->GetIOService();
}

void AsioIOServicePool::Stop()
{
    _pool->Stop();
}

bool AsioIOServicePool::Ready() const noexcept
{
    return _pool->Ready();
}

const std::string& AsioIOServicePool::startupError() const noexcept
{
    return _pool->startupError();
}
