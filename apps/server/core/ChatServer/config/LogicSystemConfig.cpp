#include "LogicSystemConfig.hpp"

#include "ConfigMgr.hpp"

#include <algorithm>
#include <string>

std::size_t
LogicSystemConfig::WorkerCount(std::size_t default_value, std::size_t min_value, std::size_t max_value) const
{
    const auto raw = ConfigMgr::Inst().GetValue("LogicSystem", "WorkerCount");
    if (raw.empty())
    {
        return default_value;
    }
    try
    {
        const auto value = static_cast<std::size_t>(std::stoul(raw));
        return std::clamp(value, min_value, max_value);
    }
    catch (...)
    {
        return default_value;
    }
}
