#include "GateDomainRuntime.hpp"

#include <cstdlib>

import memochat.gate.domain_runtime_algorithms;

unsigned short SelectGateDomainListenPort(const std::string& configured_port, unsigned short default_port)
{
    return memochat::gate::domain::modules::SelectListenPort(!configured_port.empty(),
                                                             std::atoi(configured_port.c_str()),
                                                             default_port);
}

unsigned int NormalizeGateDomainIoThreads(unsigned int hardware_concurrency)
{
    return memochat::gate::domain::modules::NormalizeIoThreadCount(hardware_concurrency);
}

unsigned int SelectGateDomainWorkerThreads(const std::string& configured_worker_threads,
                                           unsigned int fallback_worker_threads)
{
    const unsigned int parsed_threads = configured_worker_threads.empty()
                                            ? fallback_worker_threads
                                            : static_cast<unsigned int>(std::atoi(configured_worker_threads.c_str()));
    return memochat::gate::domain::modules::SelectWorkerThreadCount(parsed_threads, fallback_worker_threads);
}
