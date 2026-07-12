#include "LogicSystemConfig.hpp"

#include "ConfigMgr.hpp"

#include <algorithm>
#include <charconv>
#include <string>

std::size_t
LogicSystemConfig::WorkerCount(std::size_t default_value, std::size_t min_value, std::size_t max_value) const
{
    const auto raw = ConfigMgr::Inst().GetValue("LogicSystem", "WorkerCount");
    if (raw.empty())
    {
        return default_value;
    }
    std::size_t value = 0;
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != raw.data() + raw.size())
    {
        return default_value;
    }
    return std::clamp(value, min_value, max_value);
}
