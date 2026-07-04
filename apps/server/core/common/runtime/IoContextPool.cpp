#include "runtime/IoContextPool.hpp"

#include <iostream>

import memochat.runtime.io_context_pool_algorithms;

namespace memochat::runtime
{

IoContextPool::IoContextPool(std::size_t size)
    : _ioServices(static_cast<std::size_t>(modules::NormalizeIoContextPoolSize(size)))
    , _works(_ioServices.size())
{
    for (std::size_t i = 0; i < _ioServices.size(); ++i)
    {
        _works[i] = std::make_unique<Work>(_ioServices[i].get_executor());
    }

    for (std::size_t i = 0; i < _ioServices.size(); ++i)
    {
        _threads.emplace_back(
            [this, i]()
            {
                _ioServices[i].run();
            });
    }
}

IoContextPool::~IoContextPool()
{
    Stop();
    std::cout << "IoContextPool destruct" << std::endl;
}

boost::asio::io_context& IoContextPool::GetIOService()
{
    auto& service = _ioServices[_nextIOService++];
    if (modules::ShouldWrapNextIoContextIndex(_nextIOService, _ioServices.size()))
    {
        _nextIOService = 0;
    }
    return service;
}

void IoContextPool::Stop()
{
    if (!modules::ShouldStopIoContextPool(_stopped))
    {
        return;
    }
    _stopped = true;

    for (auto& ioService : _ioServices)
    {
        ioService.stop();
    }

    for (auto& work : _works)
    {
        work.reset();
    }

    for (auto& thread : _threads)
    {
        if (modules::ShouldJoinIoContextThread(thread.joinable()))
        {
            thread.join();
        }
    }
}

} // namespace memochat::runtime
