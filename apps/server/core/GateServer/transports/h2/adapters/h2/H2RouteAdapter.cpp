#include "WinCompat.h"
#include "adapters/h2/H2RouteAdapter.h"

#include "logging/TraceContext.h"

#include <sstream>
#include <string>
#include <unordered_map>

namespace memochat::gate::adapters::h2
{
namespace
{

std::unordered_map<std::string, std::string> ParseQueryString(const std::string& query)
{
    std::unordered_map<std::string, std::string> parsed;
    std::stringstream stream(query);
    std::string part;
    while (std::getline(stream, part, '&'))
    {
        if (part.empty())
        {
            continue;
        }

        const auto equals = part.find('=');
        if (equals == std::string::npos)
        {
            parsed[part] = "";
            continue;
        }

        parsed[part.substr(0, equals)] = part.substr(equals + 1);
    }
    return parsed;
}

std::string ReasonPhrase(int status)
{
    switch (status)
    {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        default:
            return "";
    }
}

} // namespace

memochat::gate::routing::GateRequest H2RouteAdapter::BuildGateRequest(const Http2Request& request)
{
    memochat::gate::routing::GateRequest gate_request;
    gate_request.method = request.method;
    gate_request.path = request.path;
    gate_request.target = request.query.empty() ? request.path : request.path + "?" + request.query;
    gate_request.query = ParseQueryString(request.query);
    gate_request.headers = request.headers;
    gate_request.body = request.body;
    gate_request.trace_id = request.trace_id;
    gate_request.remote_address = request.remote_addr;
    return gate_request;
}

void H2RouteAdapter::ApplyGateResponse(const memochat::gate::routing::GateResponse& route_response,
                                       Http2Response& response)
{
    response.SetStatus(route_response.status, ReasonPhrase(route_response.status));
    if (!route_response.content_type.empty())
    {
        response.content_type = route_response.content_type;
    }
    for (const auto& [name, value] : route_response.headers)
    {
        response.SetHeader(name, value);
    }

    if (route_response.body_kind == memochat::gate::routing::GateResponseBodyKind::File)
    {
        response.SetStatus(501, "Not Implemented");
        response.content_type = "application/json";
        response.body = R"({"error":501,"message":"H2 file responses are not supported by the G3 adapter"})";
        return;
    }

    if (route_response.body_kind == memochat::gate::routing::GateResponseBodyKind::Inline)
    {
        response.body = route_response.body;
    }
}

bool H2RouteAdapter::Dispatch(const Http2Request& request,
                              Http2Response& response,
                              const memochat::gate::routing::RouteRegistry& registry)
{
    const memochat::gate::routing::GateRequest gate_request = BuildGateRequest(request);
    memolog::TraceContext::SetTraceId(gate_request.trace_id);

    memochat::gate::routing::GateResponse route_response;
    const bool dispatched = registry.Dispatch(gate_request, route_response);
    if (dispatched)
    {
        ApplyGateResponse(route_response, response);
    }
    return dispatched;
}

} // namespace memochat::gate::adapters::h2
