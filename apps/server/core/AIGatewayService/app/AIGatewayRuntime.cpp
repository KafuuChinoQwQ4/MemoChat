#include "AIGatewayRuntime.hpp"

#include <cstdlib>

import memochat.ai.gateway_runtime_algorithms;

unsigned short SelectAIGatewayListenPort(const std::string& configured_port, unsigned short default_port)
{
    return memochat::ai::gateway::modules::SelectListenPort(!configured_port.empty(),
                                                            std::atoi(configured_port.c_str()),
                                                            default_port);
}

unsigned int NormalizeAIGatewayIoThreads(unsigned int hardware_concurrency)
{
    return memochat::ai::gateway::modules::NormalizeIoThreadCount(hardware_concurrency);
}

unsigned int SelectAIGatewayWorkerThreads(const std::string& configured_worker_threads, unsigned int fallback_threads)
{
    const unsigned int parsed_threads = configured_worker_threads.empty()
                                            ? fallback_threads
                                            : static_cast<unsigned int>(std::atoi(configured_worker_threads.c_str()));
    return memochat::ai::gateway::modules::SelectWorkerThreadCount(parsed_threads, fallback_threads);
}
