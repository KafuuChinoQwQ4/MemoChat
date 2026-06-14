#pragma once

#include "Http2Routes.h"
#include "routing/GateRequest.h"
#include "routing/GateResponse.h"
#include "routing/RouteRegistry.h"

namespace memochat::gate::adapters::h2
{

class H2RouteAdapter
{
public:
    static memochat::gate::routing::GateRequest BuildGateRequest(const Http2Request& request);

    static void ApplyGateResponse(const memochat::gate::routing::GateResponse& route_response, Http2Response& response);

    static bool Dispatch(const Http2Request& request,
                         Http2Response& response,
                         const memochat::gate::routing::RouteRegistry& registry);
};

} // namespace memochat::gate::adapters::h2
