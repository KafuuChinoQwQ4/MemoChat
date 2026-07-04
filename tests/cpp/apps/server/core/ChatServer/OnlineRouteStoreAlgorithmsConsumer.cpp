import memochat.chat.online_route_store_algorithms;

bool MemoChatTestOnlineRouteValidUid(int uid)
{
    return memochat::chat::persistence::online_route_store::modules::IsValidUid(uid);
}

bool MemoChatTestOnlineRouteCheckRepairSession(int uid, bool has_session)
{
    return memochat::chat::persistence::online_route_store::modules::ShouldCheckRepairSession(uid, has_session);
}

bool MemoChatTestOnlineRouteUsableServerName(bool server_name_empty)
{
    return memochat::chat::persistence::online_route_store::modules::HasUsableServerName(server_name_empty);
}

bool MemoChatTestOnlineRouteReadUserRoute(int uid)
{
    return memochat::chat::persistence::online_route_store::modules::ShouldReadUserRoute(uid);
}

bool MemoChatTestOnlineRouteSearchOnlineSets(int uid)
{
    return memochat::chat::persistence::online_route_store::modules::ShouldSearchOnlineSets(uid);
}

bool MemoChatTestOnlineRouteUseOnlineSetHit(bool contains_uid)
{
    return memochat::chat::persistence::online_route_store::modules::ShouldUseOnlineSetHit(contains_uid);
}

bool MemoChatTestOnlineRouteWriteUserRoute(int uid, bool server_name_empty)
{
    return memochat::chat::persistence::online_route_store::modules::ShouldWriteUserRoute(uid, server_name_empty);
}

bool MemoChatTestOnlineRouteClearTrackedRoute(int uid, bool server_name_empty)
{
    return memochat::chat::persistence::online_route_store::modules::ShouldClearTrackedRoute(uid, server_name_empty);
}
