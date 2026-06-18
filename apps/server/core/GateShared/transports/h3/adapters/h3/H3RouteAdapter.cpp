#include "adapters/h3/H3RouteAdapter.h"

#include "GateHttp3Connection.h"
#include "logging/TraceContext.h"

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace memochat::gate::adapters::h3
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

std::optional<std::string> ReadFileBody(const std::string& file_path)
{
    std::ifstream input(file_path, std::ios::binary);
    if (!input)
    {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof())
    {
        return std::nullopt;
    }
    return buffer.str();
}

} // namespace

memochat::gate::routing::GateRequest
H3RouteAdapter::BuildGateRequest(const std::shared_ptr<GateHttp3Connection>& connection)
{
    memochat::gate::routing::GateRequest request;
    if (!connection)
    {
        return request;
    }

    const std::string query = connection->GetQueryString();
    request.method = connection->GetRequestMethod();
    request.path = connection->GetRequestPath();
    request.target = query.empty() ? request.path : request.path + "?" + query;
    request.query = ParseQueryString(query);
    request.headers = connection->GetRequestHeaders();
    request.body = connection->GetRequestBody();
    request.trace_id = connection->GetTraceId();
    request.request_id = connection->GetRequestId();
    return request;
}

void H3RouteAdapter::ApplyGateResponse(const memochat::gate::routing::GateResponse& route_response,
                                       const std::shared_ptr<GateHttp3Connection>& connection)
{
    if (!connection)
    {
        return;
    }

    if (route_response.body_kind == memochat::gate::routing::GateResponseBodyKind::File)
    {
        const std::optional<std::string> file_body = ReadFileBody(route_response.file_path);
        if (!file_body)
        {
            connection->SendResponse(404, R"({"error":404,"message":"file response body not found"})", "application/json");
            return;
        }

        const std::string content_type =
            route_response.content_type.empty() ? "application/octet-stream" : route_response.content_type;
        connection->SendResponse(route_response.status, *file_body, content_type, route_response.headers);
        return;
    }

    if (route_response.body_kind == memochat::gate::routing::GateResponseBodyKind::Inline)
    {
        const std::string content_type =
            route_response.content_type.empty() ? "application/json" : route_response.content_type;
        connection->SendResponse(route_response.status, route_response.body, content_type, route_response.headers);
    }
}

bool H3RouteAdapter::Dispatch(const std::shared_ptr<GateHttp3Connection>& connection,
                              const memochat::gate::routing::RouteRegistry& registry)
{
    if (!connection)
    {
        return false;
    }

    const memochat::gate::routing::GateRequest request = BuildGateRequest(connection);
    memolog::TraceContext::SetTraceId(request.trace_id);
    memolog::TraceContext::SetRequestId(request.request_id);

    memochat::gate::routing::GateResponse response;
    const bool dispatched = registry.Dispatch(request, response);
    if (dispatched)
    {
        ApplyGateResponse(response, connection);
    }
    return dispatched;
}

} // namespace memochat::gate::adapters::h3
