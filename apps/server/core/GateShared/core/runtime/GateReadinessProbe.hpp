#pragma once

#include <functional>
#include <string>
#include <vector>

// Transport- and manager-agnostic startup readiness probe.
//
// Each infra slice that owns a startup-critical dependency exposes a factory
// that returns one of these (Postgres/Mongo in GateInfraPersistence, Redis in
// GateInfraCache). A service's main() assembles the probes it actually needs and
// hands them to RunGateDomainServer.
//
// The point: the domain-server bootstrap (compiled into the shared framework
// GateRuntimeCore) checks these WITHOUT including any concrete manager header.
// That is what lets the framework drop its hard link on the persistence/cache
// slices, so a service that opens no database no longer force-links
// libpq/mongoc/bson just to satisfy the shared bootstrap. Adding a new
// startup-critical dependency means adding a probe factory in its owning slice,
// not a new #include + special-case branch in the shared framework.
struct GateReadinessProbe
{
    // Human-readable dependency name, used to build the startup error message.
    std::string name;
    // Returns true when the dependency is ready. On false, writes the reason
    // into *error when error != nullptr.
    std::function<bool(std::string* error)> check;
};

using GateReadinessProbes = std::vector<GateReadinessProbe>;
