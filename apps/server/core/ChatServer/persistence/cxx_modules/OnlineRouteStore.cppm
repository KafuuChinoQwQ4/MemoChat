export module memochat.chat.online_route_store_algorithms;

export namespace memochat::chat::persistence::online_route_store::modules
{
bool IsValidUid(int uid)
{
    return uid > 0;
}

bool ShouldCheckRepairSession(int uid, bool has_session)
{
    return IsValidUid(uid) && has_session;
}

bool HasUsableServerName(bool server_name_empty)
{
    return !server_name_empty;
}

bool ShouldReadUserRoute(int uid)
{
    return IsValidUid(uid);
}

bool ShouldSearchOnlineSets(int uid)
{
    return IsValidUid(uid);
}

bool ShouldUseOnlineSetHit(bool contains_uid)
{
    return contains_uid;
}

bool ShouldWriteUserRoute(int uid, bool server_name_empty)
{
    return IsValidUid(uid) && HasUsableServerName(server_name_empty);
}

bool ShouldClearTrackedRoute(int uid, bool server_name_empty)
{
    return IsValidUid(uid) && HasUsableServerName(server_name_empty);
}
} // namespace memochat::chat::persistence::online_route_store::modules
