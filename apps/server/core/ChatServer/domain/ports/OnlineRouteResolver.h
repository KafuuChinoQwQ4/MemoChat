#pragma once

#include "ISessionRegistry.h"
#include "IOnlineRouteStore.h"

#include <memory>
#include <string>

class CSession;

namespace memochat::chat::routing
{

// Outcome of resolving where a recipient uid is currently reachable.
//   Offline  — no live session and no tracked online route.
//   Local    — bound to a session on this node (carried in `session`).
//   Remote   — online on a different node (`redis_server` names it).
//   Stale    — a tracked route pointed back at this node but no local
//              session exists; the route has been cleared.
enum class OnlineRouteKind
{
    Offline,
    Local,
    Remote,
    Stale
};

struct OnlineRouteDecision
{
    OnlineRouteKind kind = OnlineRouteKind::Offline;
    std::shared_ptr<CSession> session;
    std::string redis_server;
    bool local_session_found = false;
};

// PURE decision: "where is uid reachable right now?" — reads the session-registry
// and online-route-store seams and returns the decision, performing NO writes.
// This is the test surface: a fake store can assert DecideOnlineRoute never
// mutates across all four kinds. Most callers want ResolveOnlineRoute (decide +
// self-heal); reach for this directly only when the self-healing writes must be
// suppressed or applied separately.
inline OnlineRouteDecision
DecideOnlineRoute(int uid, ISessionRegistry* session_registry, IOnlineRouteStore* online_route_store)
{
    OnlineRouteDecision route;
    if (uid <= 0 || !session_registry || !online_route_store)
    {
        return route;
    }

    const auto self_name = online_route_store->SelfServerName();
    auto session = session_registry->FindSession(uid);
    if (session)
    {
        route.kind = OnlineRouteKind::Local;
        route.session = session;
        route.redis_server = self_name;
        route.local_session_found = true;
        return route;
    }

    auto redis_server = online_route_store->FindUserServer(uid);
    if (redis_server.empty())
    {
        redis_server = online_route_store->ResolveServerFromOnlineSets(uid);
        if (redis_server.empty())
        {
            return route;
        }
    }
    route.redis_server = redis_server;
    if (redis_server != self_name)
    {
        route.kind = OnlineRouteKind::Remote;
        return route;
    }

    const auto reloaded_server = online_route_store->FindUserServer(uid);
    if (!reloaded_server.empty())
    {
        route.redis_server = reloaded_server;
        if (reloaded_server != self_name)
        {
            route.kind = OnlineRouteKind::Remote;
            return route;
        }
    }

    route.kind = OnlineRouteKind::Stale;
    return route;
}

// Self-healing writes implied by a resolved route, applied as an explicit, named
// step (no longer buried inside the decision branches): a Local route re-anchors
// the tracked online route to this node; a Stale route clears the dangling track.
// Idempotent and safe to call once per decision; Remote/Offline are no-ops.
inline void ApplyOnlineRouteSelfHeal(const OnlineRouteDecision& route, int uid, IOnlineRouteStore* online_route_store)
{
    if (uid <= 0 || !online_route_store)
    {
        return;
    }
    if (route.kind == OnlineRouteKind::Local && route.session)
    {
        const auto& session = route.session;
        online_route_store->RepairOnlineRoute(uid, session);
    }
    else if (route.kind == OnlineRouteKind::Stale)
    {
        const auto self_name = online_route_store->SelfServerName();
        online_route_store->ClearTrackedOnlineRoute(uid, self_name);
    }
}

// Single source of truth for "where is uid reachable right now?" used by every
// delivery path (synchronous push, async event fan-out, ...). Deep entry that
// composes the pure DecideOnlineRoute with the self-healing route repair/clear
// the online-route-store seam owns — preserving the original behavior (Local
// re-anchors the route, Stale clears it) in one place. Callers branch on `kind`.
inline OnlineRouteDecision
ResolveOnlineRoute(int uid, ISessionRegistry* session_registry, IOnlineRouteStore* online_route_store)
{
    OnlineRouteDecision route = DecideOnlineRoute(uid, session_registry, online_route_store);
    ApplyOnlineRouteSelfHeal(route, uid, online_route_store);
    return route;
}

// Stable wire/log label for a resolved route kind.
inline const char* OnlineRouteResultName(OnlineRouteKind kind)
{
    switch (kind)
    {
        case OnlineRouteKind::Local:
            return "local";
        case OnlineRouteKind::Remote:
            return "remote";
        case OnlineRouteKind::Stale:
            return "stale";
        case OnlineRouteKind::Offline:
        default:
            return "offline";
    }
}

} // namespace memochat::chat::routing
