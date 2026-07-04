#pragma once

#include <string>

unsigned short SelectAIGatewayListenPort(const std::string& configured_port, unsigned short default_port);
unsigned int NormalizeAIGatewayIoThreads(unsigned int hardware_concurrency);
unsigned int SelectAIGatewayWorkerThreads(const std::string& configured_worker_threads, unsigned int fallback_threads);
