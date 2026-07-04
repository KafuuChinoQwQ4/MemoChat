#pragma once

#include "routing/GateRequest.hpp"
#include "routing/GateResponse.hpp"
#include "routing/RouteRegistry.hpp"

#include <memory>

class GateHttp3Connection;

namespace memochat::gate::adapters::h3
{

class H3RouteAdapter
{
public:
    static memochat::gate::routing::GateRequest
    BuildGateRequest(const std::shared_ptr<GateHttp3Connection>& connection);

    static void ApplyGateResponse(const memochat::gate::routing::GateResponse& route_response,
                                  const std::shared_ptr<GateHttp3Connection>& connection);

    static bool Dispatch(const std::shared_ptr<GateHttp3Connection>& connection,
                         const memochat::gate::routing::RouteRegistry& registry);
};

} // namespace memochat::gate::adapters::h3
