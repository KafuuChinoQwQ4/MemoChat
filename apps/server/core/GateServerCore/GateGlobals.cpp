#include "GateGlobals.h"

namespace gateglobals {
    boost::asio::io_context* g_main_ioc = nullptr;
    unsigned int g_worker_threads = 0;
}
