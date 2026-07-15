#pragma once

#include "runtime/GateReadinessProbe.hpp"

// Cache-slice readiness probes. Defined in GateInfraCache (next to RedisMgr) so
// the shared GateRuntimeCore bootstrap never references the concrete RedisMgr
// singleton — the framework only sees the transport-agnostic GateReadinessProbe
// contract, and a service opts into the Redis check by linking GateInfraCache
// and passing this probe. See runtime/GateReadinessProbe.hpp for the rationale.
namespace memochat::gate::cache
{

// Probe reporting whether the process-wide RedisMgr singleton opened its
// connection pool from config. Named "Redis" so startup errors read identically
// to the previous inline check ("Redis: <error>").
GateReadinessProbe RedisReadinessProbe();

} // namespace memochat::gate::cache
