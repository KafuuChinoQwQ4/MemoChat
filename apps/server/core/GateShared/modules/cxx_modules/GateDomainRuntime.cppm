export module memochat.gate.domain_runtime_algorithms;

import std;

export namespace memochat::gate::domain::modules
{
unsigned short SelectListenPort(bool has_configured_port, int configured_port, unsigned short default_port)
{
    return has_configured_port ? static_cast<unsigned short>(configured_port) : default_port;
}

unsigned int NormalizeIoThreadCount(unsigned int hardware_concurrency)
{
    return std::cmp_less(hardware_concurrency, 2u) ? 4u : hardware_concurrency;
}

unsigned int SelectWorkerThreadCount(unsigned int configured_worker_threads, unsigned int fallback_worker_threads)
{
    if (configured_worker_threads < 1)
    {
        return fallback_worker_threads;
    }
    return configured_worker_threads;
}
} // namespace memochat::gate::domain::modules
