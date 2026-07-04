#include <string_view>

import memochat.ai.route_module_algorithms;

namespace memochat::gate::modules::ai::testsupport
{
int NormalizeRouteTimeoutSeconds(int value)
{
    return route_algorithms::AtLeast(value, 1);
}

bool RouteTargetStartsWith(std::string_view target, std::string_view prefix)
{
    return route_algorithms::StartsWith(target.data(), target.size(), prefix.data(), prefix.size());
}
} // namespace memochat::gate::modules::ai::testsupport
