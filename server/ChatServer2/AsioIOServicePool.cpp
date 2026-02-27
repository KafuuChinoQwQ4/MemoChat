#include "AsioIOServicePool.h"
#include <iostream>

using namespace std;

AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _ioServices(size), _works(size), _nextIOService(0)
{
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::make_unique<Work>(_ioServices[i].get_executor());
    }

    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
        });
    }
}

AsioIOServicePool::~AsioIOServicePool()
{
    std::cout << "AsioIOServicePool destruct" << endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService()
{
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) {
        _nextIOService = 0;
    }
    return service;
}

void AsioIOServicePool::Stop()
{
    for (auto& ioService : _ioServices) {
        ioService.stop();
    }

    for (auto& work : _works) {
        work.reset();
    }

    for (auto& t : _threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}
