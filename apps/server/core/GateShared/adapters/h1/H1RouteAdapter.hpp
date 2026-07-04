#pragma once

#include "routing/GateRequest.hpp"
#include "routing/GateResponse.hpp"

#include <memory>
#include <string>

class HttpConnection;

namespace memochat::gate::adapters::h1
{

class H1RouteAdapter
{
public:
    static memochat::gate::routing::GateRequest BuildGateRequest(const std::string& method,
                                                                 const std::string& path,
                                                                 const std::shared_ptr<HttpConnection>& connection);

    static void ApplyGateResponse(const memochat::gate::routing::GateResponse& route_response,
                                  const std::shared_ptr<HttpConnection>& connection);
};

} // namespace memochat::gate::adapters::h1
