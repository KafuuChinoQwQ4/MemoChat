#pragma once

#include "LogicSystem.h"
#include <string>

// Shared entrypoint body for the focused single-domain gateway processes peeled
// off GateServer (gateserver microservice split, Phase 3/4). Each domain exe
// (MediaGatewayServer, MomentsGatewayServer, CallGatewayServer, R18GatewayServer)
// is a thin main() that calls RunGateDomainServer() with its route profile,
// config section and default port. The body mirrors GateServer.cpp's H1 bootstrap
// but selects LogicSystem::RouteProfile so the process serves only its own domain
// routes (+ health). It initializes the worker pool + Snowflake (+ AWS for media)
// and relies on the lazy Redis/Postgres/Mongo singletons that the domain services
// open from config on first use.
int RunGateDomainServer(LogicSystem::RouteProfile profile,
                        const std::string& service_name,
                        const std::string& config_section,
                        unsigned short default_port,
                        bool init_aws,
                        bool init_async = false);
