#pragma once

#include <boost/asio/io_context.hpp>
#include <cstddef>

namespace h1globals {
    extern boost::asio::io_context* g_main_ioc;
    extern std::size_t g_worker_threads;
}
