#include "LogicSystem.hpp"
#include "HttpConnection.hpp"
#include "adapters/h1/H1RouteAdapter.hpp"
#include "GateHttpJsonSupport.hpp"
#include "modules/health/HealthRouteModule.hpp"
#include "transports/h3/listener/GateHttp3Connection.hpp"
#include "logging/Logger.hpp"
#include "logging/TraceContext.hpp"
#include <string_view>

import memochat.gate.routing_algorithms;

namespace
{
bool MatchesRoutePrefix(const std::string& path, const std::string& prefix)
{
    return memochat::gate::routing::modules::MatchesRoutePrefix(path.data(), path.size(), prefix.data(), prefix.size());
}

void ApplyRouteTraceContext(const memochat::gate::routing::GateRequest& request)
{
    memolog::TraceContext::SetTraceId(request.trace_id);
    memolog::TraceContext::SetRequestId(request.request_id);
}

} // namespace

namespace
{
std::vector<LogicSystem::RouteProfileRegistrar>& RouteProfileRegistrars()
{
    static std::vector<LogicSystem::RouteProfileRegistrar> registrars;
    return registrars;
}
} // namespace

void LogicSystem::AddRouteProfileRegistrar(RouteProfileRegistrar registrar)
{
    RouteProfileRegistrars().push_back(std::move(registrar));
}

void LogicSystem::ClearRouteProfileRegistrars()
{
    RouteProfileRegistrars().clear();
}

LogicSystem::LogicSystem()
{
    // Health is always registered (every process exposes /healthz, /readyz).
    memochat::gate::modules::health::HealthRouteModule().RegisterRoutes(_route_registry);

    for (const auto& registrar : RouteProfileRegistrars())
    {
        registrar(_route_registry);
    }
}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
    _get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegGetPrefix(std::string url, HttpHandler handler)
{
    _get_prefix_handlers.emplace_back(std::move(url), std::move(handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
    _post_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPostPrefix(std::string url, HttpHandler handler)
{
    _post_prefix_handlers.emplace_back(std::move(url), std::move(handler));
}

void LogicSystem::RegDelete(std::string url, HttpHandler handler)
{
    _delete_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegDeletePrefix(std::string url, HttpHandler handler)
{
    _delete_prefix_handlers.emplace_back(std::move(url), std::move(handler));
}

LogicSystem::~LogicSystem()
{
}

bool LogicSystem::DispatchH1(const std::string& path,
                             std::shared_ptr<HttpConnection> con,
                             const std::map<std::string, HttpHandler>& exact,
                             const std::vector<std::pair<std::string, HttpHandler>>& prefix,
                             std::string_view method,
                             std::string_view dispatch_event,
                             std::string_view prefix_event,
                             std::string_view not_found_event)
{
    auto it = exact.find(path);
    if (it != exact.end())
    {
        if (!dispatch_event.empty())
        {
            memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
            memolog::LogInfo(std::string(dispatch_event), "dispatch route", {{"route", path}});
        }
        it->second(con);
        return true;
    }

    for (const auto& [pfx, handler] : prefix)
    {
        if (MatchesRoutePrefix(path, pfx))
        {
            if (!prefix_event.empty())
            {
                memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
                memolog::LogInfo(std::string(prefix_event),
                                 "dispatch prefix route",
                                 {{"route", path}, {"prefix", pfx}});
            }
            handler(con);
            return true;
        }
    }

    memochat::gate::routing::GateResponse response;
    const auto request = memochat::gate::adapters::h1::H1RouteAdapter::BuildGateRequest(std::string(method), path, con);
    ApplyRouteTraceContext(request);
    if (_route_registry.Dispatch(request, response))
    {
        memochat::gate::adapters::h1::H1RouteAdapter::ApplyGateResponse(response, con);
        return true;
    }

    if (!not_found_event.empty())
    {
        memolog::LogWarn(std::string(not_found_event), "route not found", {{"route", path}});
    }
    return false;
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con)
{
    return DispatchH1(path, con, _get_handlers, _get_prefix_handlers, "GET", "", "", "");
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con)
{
    return DispatchH1(path,
                      con,
                      _post_handlers,
                      _post_prefix_handlers,
                      "POST",
                      "gate.http.post.dispatch",
                      "gate.http.post.dispatch_prefix",
                      "gate.http.post.not_found");
}

bool LogicSystem::HandleDelete(std::string path, std::shared_ptr<HttpConnection> con)
{
    return DispatchH1(path,
                      con,
                      _delete_handlers,
                      _delete_prefix_handlers,
                      "DELETE",
                      "gate.http.delete.dispatch",
                      "gate.http.delete.dispatch_prefix",
                      "gate.http.delete.not_found");
}

void LogicSystem::RegGet(std::string url, Http3Handler handler)
{
    _get3_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, Http3Handler handler)
{
    _post3_handlers.insert(make_pair(url, handler));
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<GateHttp3Connection> con)
{
    if (_get3_handlers.find(path) == _get3_handlers.end())
    {
        return false;
    }
    memolog::TraceContext::SetTraceId(con ? con->GetTraceId() : "");
    memolog::LogInfo("gate.http3.get.dispatch", "dispatch HTTP/3 GET route", {{"route", path}});
    _get3_handlers[path](con);
    return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<GateHttp3Connection> con)
{
    if (_post3_handlers.find(path) == _post3_handlers.end())
    {
        memolog::LogWarn("gate.http3.post.not_found", "HTTP/3 post route not found", {{"route", path}});
        return false;
    }
    memolog::TraceContext::SetTraceId(con ? con->GetTraceId() : "");
    memolog::LogInfo("gate.http3.post.dispatch", "dispatch HTTP/3 POST route", {{"route", path}});
    _post3_handlers[path](con);
    return true;
}
