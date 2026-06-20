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

// Single source of truth for "where is uid reachable right now?" used by every
// delivery path (synchronous push, async event fan-out, ...). Pure decision
// logic over the session-registry and online-route-store seams; performs the
// self-healing route repair/clear side effects those seams already own, but no
// I/O of its own. Callers branch on `kind` and send accordingly.
inline OnlineRouteDecision
ResolveOnlineRoute(int uid, ISessionRegistry* session_registry, IOnlineRouteStore* online_route_store)
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
        online_route_store->RepairOnlineRoute(uid, session);
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
    online_route_store->ClearTrackedOnlineRoute(uid, self_name);
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
