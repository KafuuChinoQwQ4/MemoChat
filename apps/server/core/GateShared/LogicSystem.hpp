#pragma once
#include "Singleton.hpp"
#include <functional>
#include <map>
#include <vector>
#include "const.hpp"
#include "routing/RouteRegistry.hpp"

class HttpConnection;
class GateHttp3Connection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
typedef std::function<void(std::shared_ptr<GateHttp3Connection>)> Http3Handler;
class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;

public:
    using RouteProfileRegistrar = std::function<void(memochat::gate::routing::RouteRegistry&)>;

    static void AddRouteProfileRegistrar(RouteProfileRegistrar registrar);
    static void ClearRouteProfileRegistrars();

    ~LogicSystem();
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
    bool HandleGet(std::string, std::shared_ptr<GateHttp3Connection>);
    void RegGet(std::string, HttpHandler handler);
    void RegGetPrefix(std::string, HttpHandler handler);
    void RegGet(std::string, Http3Handler handler);
    void RegPost(std::string, HttpHandler handler);
    void RegPostPrefix(std::string, HttpHandler handler);
    void RegDelete(std::string, HttpHandler handler);
    void RegDeletePrefix(std::string, HttpHandler handler);
    void RegPost(std::string, Http3Handler handler);
    bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
    bool HandleDelete(std::string, std::shared_ptr<HttpConnection>);
    bool HandlePost(std::string, std::shared_ptr<GateHttp3Connection>);

private:
    LogicSystem();
    bool DispatchH1(const std::string& path,
                    std::shared_ptr<HttpConnection> con,
                    const std::map<std::string, HttpHandler>& exact,
                    const std::vector<std::pair<std::string, HttpHandler>>& prefix,
                    std::string_view method,
                    std::string_view dispatch_event,
                    std::string_view prefix_event,
                    std::string_view not_found_event);
    std::map<std::string, HttpHandler> _post_handlers;
    std::map<std::string, HttpHandler> _get_handlers;
    std::map<std::string, HttpHandler> _delete_handlers;
    std::vector<std::pair<std::string, HttpHandler>> _post_prefix_handlers;
    std::vector<std::pair<std::string, HttpHandler>> _get_prefix_handlers;
    std::vector<std::pair<std::string, HttpHandler>> _delete_prefix_handlers;
    std::map<std::string, Http3Handler> _post3_handlers;
    std::map<std::string, Http3Handler> _get3_handlers;
    memochat::gate::routing::RouteRegistry _route_registry;
};
