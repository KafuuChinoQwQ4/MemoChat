#pragma once
#include <boost/asio.hpp>
#include <memory>

namespace gateglobals {
    extern boost::asio::io_context* g_main_ioc;
    extern unsigned int g_worker_threads;
}
