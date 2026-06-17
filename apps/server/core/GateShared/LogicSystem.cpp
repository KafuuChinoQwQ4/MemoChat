#include "LogicSystem.h"
#include "HttpConnection.h"
#include "adapters/h1/H1RouteAdapter.h"
#include "GateHttpJsonSupport.h"
#include "modules/health/HealthRouteModule.h"
#include "transports/h3/listener/GateHttp3Connection.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

namespace
{
bool MatchesRoutePrefix(const std::string& path, const std::string& prefix)
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

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con)
{
    auto exact = _get_handlers.find(path);
    if (exact != _get_handlers.end())
    {
        exact->second(con);
        return true;
    }

    for (const auto& [prefix, handler] : _get_prefix_handlers)
    {
        if (MatchesRoutePrefix(path, prefix))
        {
            handler(con);
            return true;
        }
    }

    memochat::gate::routing::GateResponse response;
    const auto request = memochat::gate::adapters::h1::H1RouteAdapter::BuildGateRequest("GET", path, con);
    ApplyRouteTraceContext(request);
    if (_route_registry.Dispatch(request, response))
    {
        memochat::gate::adapters::h1::H1RouteAdapter::ApplyGateResponse(response, con);
        return true;
    }

    return false;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con)
{
    auto exact = _post_handlers.find(path);
    if (exact != _post_handlers.end())
    {
        memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
        memolog::LogInfo("gate.http.post.dispatch", "dispatch post route", {{"route", path}});
        exact->second(con);
        return true;
    }

    for (const auto& [prefix, handler] : _post_prefix_handlers)
    {
        if (MatchesRoutePrefix(path, prefix))
        {
            memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
            memolog::LogInfo("gate.http.post.dispatch_prefix",
                             "dispatch post prefix route",
                             {{"route", path}, {"prefix", prefix}});
            handler(con);
            return true;
        }
    }

    memochat::gate::routing::GateResponse response;
    const auto request = memochat::gate::adapters::h1::H1RouteAdapter::BuildGateRequest("POST", path, con);
    ApplyRouteTraceContext(request);
    if (_route_registry.Dispatch(request, response))
    {
        memochat::gate::adapters::h1::H1RouteAdapter::ApplyGateResponse(response, con);
        return true;
    }

    memolog::LogWarn("gate.http.post.not_found", "post route not found", {{"route", path}});
    return false;
}

bool LogicSystem::HandleDelete(std::string path, std::shared_ptr<HttpConnection> con)
{
    auto exact = _delete_handlers.find(path);
    if (exact != _delete_handlers.end())
    {
        memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
        memolog::LogInfo("gate.http.delete.dispatch", "dispatch delete route", {{"route", path}});
        exact->second(con);
        return true;
    }

    for (const auto& [prefix, handler] : _delete_prefix_handlers)
    {
        if (MatchesRoutePrefix(path, prefix))
        {
            memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
            memolog::LogInfo("gate.http.delete.dispatch_prefix",
                             "dispatch delete prefix route",
                             {{"route", path}, {"prefix", prefix}});
            handler(con);
            return true;
        }
    }

    memochat::gate::routing::GateResponse response;
    const auto request = memochat::gate::adapters::h1::H1RouteAdapter::BuildGateRequest("DELETE", path, con);
    ApplyRouteTraceContext(request);
    if (_route_registry.Dispatch(request, response))
    {
        memochat::gate::adapters::h1::H1RouteAdapter::ApplyGateResponse(response, con);
        return true;
    }

    memolog::LogWarn("gate.http.delete.not_found", "delete route not found", {{"route", path}});
    return false;
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
