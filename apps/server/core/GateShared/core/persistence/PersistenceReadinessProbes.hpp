#pragma once

#include "runtime/GateReadinessProbe.hpp"

// Readiness probes for the persistence-tier dependencies. These live in the
// GateInfraPersistence slice so the Postgres/Mongo manager singletons are only
// referenced by code that a service explicitly opts into by linking this slice.
// A service main() that needs Postgres/Mongo adds the matching probe to the
// vector it passes to RunGateDomainServer; the shared framework never names the
// concrete manager itself.
namespace memochat::gate::persistence
{

// Probe that reports whether the Postgres manager singleton opened successfully.
GateReadinessProbe PostgresReadinessProbe();

// Probe that reports whether the Mongo manager singleton opened successfully.
GateReadinessProbe MongoReadinessProbe();

} // namespace memochat::gate::persistence
