#pragma once

#include "routing/RouteRegistry.h"
#include <functional>
#include <string>

using GateDomainRouteRegistrar = void (*)(memochat::gate::routing::RouteRegistry&);
using GateDomainLifecycleHook = std::function<void()>;

// Shared entrypoint body for the focused single-domain gateway processes peeled
// off GateServer (gateserver microservice split, Phase 3/4). Each domain exe
// (MediaGatewayServer, MomentsGatewayServer, CallGatewayServer, R18GatewayServer)
// is a thin main() that calls RunGateDomainServer() with its route profile,
// config section and default port. The body mirrors GateServer.cpp's H1 bootstrap
// but selects LogicSystem::RouteProfile so the process serves only its own domain
// routes (+ health). It initializes the worker pool + Snowflake (+ AWS for media)
// and relies on the lazy Redis/Postgres/Mongo singletons that the domain services
// open from config on first use.
int RunGateDomainServer(GateDomainRouteRegistrar registrar,
                        const std::string& service_name,
                        const std::string& config_section,
                        unsigned short default_port,
                        bool init_aws,
                        GateDomainLifecycleHook on_start = {},
                        GateDomainLifecycleHook on_stop = {});
