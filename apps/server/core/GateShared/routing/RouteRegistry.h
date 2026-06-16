#pragma once

#include "routing/GateRequest.h"
#include "routing/GateResponse.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace memochat::gate::routing
{

class RouteRegistry
{
public:
    using RouteHandler = std::function<bool(const GateRequest&, GateResponse&)>;

    void Register(std::string method, std::string path, RouteHandler handler);
    void RegisterPrefix(std::string method, std::string prefix, RouteHandler handler);
    bool Dispatch(const GateRequest& request, GateResponse& response) const;

private:
    struct PrefixRoute
    {
        std::string method;
        std::string prefix;
        RouteHandler handler;
    };

    static std::string NormalizeMethod(std::string method);
    static std::string RouteKey(const std::string& method, const std::string& path);
    static bool MatchesRoutePrefix(const std::string& path, const std::string& prefix);

    std::unordered_map<std::string, RouteHandler> exact_routes_;
    std::vector<PrefixRoute> prefix_routes_;
};

} // namespace memochat::gate::routing
