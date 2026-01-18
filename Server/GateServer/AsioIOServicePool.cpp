#include "AsioIOServicePool.h"
#include <iostream>

AsioIOServicePool::AsioIOServicePool(std::size_t size) : _ioServices(size), _works(size), _nextIOService(0) {
    for (std::size_t i = 0; i < size; ++i) {
        // [修复] 使用 make_work_guard 创建 executor_work_guard
        // Work 的类型现在是 boost::asio::executor_work_guard<...>
        _works[i] = std::unique_ptr<Work>(new Work(boost::asio::make_work_guard(_ioServices[i])));
    }

    // 创建线程跑 io_context
    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
        });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) {
        _nextIOService = 0;
    }
    return service;
}

void AsioIOServicePool::Stop() {
    // [修复] 适配新的 work_guard 接口
    for (auto& work : _works) {
        // 1. 获取对应的 io_context 并停止它
        // executor_work_guard 没有 get_io_context()，需要通过 get_executor().context() 获取
        if (work) {
            work->get_executor().context().stop();
            
            // 2. 销毁 work guard (释放 unique_ptr)
            work.reset();
        }
    }

    for (auto& t : _threads) {
        if (t.joinable()) t.join();
    }
}