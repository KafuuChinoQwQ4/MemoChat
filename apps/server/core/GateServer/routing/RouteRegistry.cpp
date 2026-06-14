#include "routing/RouteRegistry.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace memochat::gate::routing
{

void RouteRegistry::Register(std::string method, std::string path, RouteHandler handler)
{
    exact_routes_[RouteKey(NormalizeMethod(std::move(method)), path)] = std::move(handler);
}

void RouteRegistry::RegisterPrefix(std::string method, std::string prefix, RouteHandler handler)
{
    prefix_routes_.push_back(PrefixRoute{NormalizeMethod(std::move(method)), std::move(prefix), std::move(handler)});
}

bool RouteRegistry::Dispatch(const GateRequest& request, GateResponse& response) const
{
    const std::string method = NormalizeMethod(request.method);
    const auto exact = exact_routes_.find(RouteKey(method, request.path));
    if (exact != exact_routes_.end())
    {
        return exact->second(request, response);
    }

    for (const auto& route : prefix_routes_)
    {
        if (route.method == method && MatchesRoutePrefix(request.path, route.prefix))
        {
            return route.handler(request, response);
        }
    }

    return false;
}

std::string RouteRegistry::NormalizeMethod(std::string method)
{
    std::transform(method.begin(),
                   method.end(),
                   method.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::toupper(ch));
                   });
    return method;
}

std::string RouteRegistry::RouteKey(const std::string& method, const std::string& path)
{
    return method + " " + path;
}

bool RouteRegistry::MatchesRoutePrefix(const std::string& path, const std::string& prefix)
{
    if (path == prefix)
    {
        return true;
    }
    if (path.size() <= prefix.size() || path.rfind(prefix, 0) != 0)
    {
        return false;
    }

    const char next = path[prefix.size()];
    return next == '/' || next == '?';
}

} // namespace memochat::gate::routing
