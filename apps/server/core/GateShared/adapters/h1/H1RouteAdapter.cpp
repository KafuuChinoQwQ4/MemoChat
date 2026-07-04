#include "adapters/h1/H1RouteAdapter.hpp"

#include "HttpConnection.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

import memochat.gate.h1_route_adapter_algorithms;

namespace memochat::gate::adapters::h1
{
namespace h1_modules = memochat::gate::adapters::h1::modules;

memochat::gate::routing::GateRequest H1RouteAdapter::BuildGateRequest(const std::string& method,
                                                                      const std::string& path,
                                                                      const std::shared_ptr<HttpConnection>& connection)
{
    memochat::gate::routing::GateRequest request;
    request.method = method;
    request.path = path;
    request.target = connection ? connection->RequestTargetString() : path;
    request.body = connection ? connection->RequestBodyString() : "";
    request.trace_id = connection ? connection->GetTraceId() : "";
    request.request_id = connection ? connection->GetRequestId() : "";
    if (h1_modules::ShouldReadConnectionFields(static_cast<bool>(connection)))
    {
        request.query = connection->GetParams();
        for (const auto& field : connection->GetRequest())
        {
            request.headers.emplace(std::string(field.name_string()), std::string(field.value()));
        }
        boost::beast::error_code ec;
        const auto remote = connection->GetSocket().remote_endpoint(ec);
        if (!ec)
        {
            request.remote_address = remote.address().to_string();
        }
    }
    return request;
}

void H1RouteAdapter::ApplyGateResponse(const memochat::gate::routing::GateResponse& route_response,
                                       const std::shared_ptr<HttpConnection>& connection)
{
    if (!connection)
    {
        return;
    }

    auto& response = connection->GetResponse();
    response.result(static_cast<boost::beast::http::status>(route_response.status));
    if (h1_modules::ShouldSetContentType(route_response.content_type.empty()))
    {
        response.set(boost::beast::http::field::content_type, route_response.content_type);
    }
    for (const auto& [name, value] : route_response.headers)
    {
        response.set(name, value);
    }
    if (h1_modules::ShouldApplyFileResponse(route_response.body_kind ==
                                            memochat::gate::routing::GateResponseBodyKind::File))
    {
        connection->SetFileResponse(route_response.file_path, route_response.content_type);
        return;
    }

    boost::beast::ostream(response.body()) << route_response.body;
}

} // namespace memochat::gate::adapters::h1
