#include "AIGatewayRuntime.hpp"

#include <charconv>

import memochat.ai.gateway_runtime_algorithms;

namespace
{
int ParseIntOrDefault(const std::string& raw, int fallback)
{
    int value = 0;
    const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return !raw.empty() && ec == std::errc{} && ptr == raw.data() + raw.size() ? value : fallback;
}
} // namespace

unsigned short SelectAIGatewayListenPort(const std::string& configured_port, unsigned short default_port)
{
    return memochat::ai::gateway::modules::SelectListenPort(!configured_port.empty(),
                                                            ParseIntOrDefault(configured_port, 0),
                                                            default_port);
}

unsigned int NormalizeAIGatewayIoThreads(unsigned int hardware_concurrency)
{
    return memochat::ai::gateway::modules::NormalizeIoThreadCount(hardware_concurrency);
}

unsigned int SelectAIGatewayWorkerThreads(const std::string& configured_worker_threads, unsigned int fallback_threads)
{
    const int parsed = ParseIntOrDefault(configured_worker_threads, static_cast<int>(fallback_threads));
    const unsigned int parsed_threads = parsed > 0 ? static_cast<unsigned int>(parsed) : fallback_threads;
    return memochat::ai::gateway::modules::SelectWorkerThreadCount(parsed_threads, fallback_threads);
}
