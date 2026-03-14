#include "AsioIOServicePool.h"

AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _pool(std::make_unique<memochat::runtime::IoContextPool>(size)) {
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
