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
            if (redis->Ready())
            {
                return true;
            }
            if (error != nullptr)
            {
                *error = redis->StartupError();
            }
            return false;
        },
    };
}

} // namespace memochat::gate::cache
