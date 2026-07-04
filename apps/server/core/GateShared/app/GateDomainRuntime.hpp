#pragma once

#include <string>

unsigned short SelectGateDomainListenPort(const std::string& configured_port, unsigned short default_port);
unsigned int NormalizeGateDomainIoThreads(unsigned int hardware_concurrency);
unsigned int SelectGateDomainWorkerThreads(const std::string& configured_worker_threads,
                                           unsigned int fallback_worker_threads);
