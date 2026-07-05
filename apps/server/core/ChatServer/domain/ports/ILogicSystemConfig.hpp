#pragma once

#include <cstddef>

// Configuration surface for LogicSystem's own runtime parameters
// (worker-thread pool size, etc.).  Concrete implementation lives in
// config/LogicSystemConfig — the domain layer never sees ConfigMgr directly.
class ILogicSystemConfig
{
public:
    virtual ~ILogicSystemConfig() = default;

    // Returns the configured worker-thread count, clamped to
    // [min_value, max_value]; falls back to default_value if the key is absent
    // or cannot be parsed.
    virtual std::size_t WorkerCount(std::size_t default_value, std::size_t min_value, std::size_t max_value) const = 0;
};
