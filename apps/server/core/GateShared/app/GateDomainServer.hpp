#pragma once

#include "runtime/GateReadinessProbe.hpp"
#include "routing/RouteRegistry.hpp"
#include <functional>
#include <string>

using GateDomainRouteRegistrar = void (*)(memochat::gate::routing::RouteRegistry&);
using GateDomainStartHook = std::function<bool(std::string*)>;
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
//
// A service declares which startup-critical dependencies must be verified before
// the listener opens by passing readiness probes (see runtime/GateReadinessProbe.hpp).
// The probe factories live in the owning infra slice — RedisReadinessProbe() in
// GateInfraCache, Postgres/MongoReadinessProbe() in GateInfraPersistence — so this
// shared bootstrap never names a concrete manager and the framework needs no link
// on the persistence/cache slices.
int RunGateDomainServer(GateDomainRouteRegistrar registrar,
                        const std::string& service_name,
                        const std::string& config_section,
                        unsigned short default_port,
                        bool init_aws,
                        GateDomainStartHook on_start = {},
                        GateDomainLifecycleHook on_stop = {},
                        GateReadinessProbes readiness_probes = {});
