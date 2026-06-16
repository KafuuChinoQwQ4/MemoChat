#pragma once

namespace memochat::gate::routing
{

class RouteRegistry;

class RouteModule
{
public:
    virtual ~RouteModule() = default;
    virtual void RegisterRoutes(RouteRegistry& registry) = 0;
};

} // namespace memochat::gate::routing
