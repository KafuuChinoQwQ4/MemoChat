#include "CacheReadinessProbes.hpp"

#include "RedisMgr.hpp"

namespace memochat::gate::cache
{

GateReadinessProbe RedisReadinessProbe()
{
    return GateReadinessProbe{
        .name = "Redis",
        .check = [](std::string* error) -> bool
        {
            const auto redis = RedisMgr::GetInstance();
            if (redis->Healthy())
            {
                return true;
            }
            if (error != nullptr)
            {
                *error = redis->Ready() ? "redis dependency is unavailable" : redis->StartupError();
            }
            return false;
        },
    };
}

} // namespace memochat::gate::cache
