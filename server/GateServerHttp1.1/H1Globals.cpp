#include "H1Globals.h"

boost::asio::io_context* h1globals::g_main_ioc = nullptr;
std::size_t h1globals::g_worker_threads = 0;
