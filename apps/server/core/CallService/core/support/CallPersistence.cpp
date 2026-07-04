#include "CallPersistence.hpp"

#include "PostgresMgr.hpp"

CallPersistence& CallPersistence::Instance()
{
    static CallPersistence instance;
    return instance;
}

bool CallPersistence::AreUsersMutualFriends(int uid, int peer_uid) const
{
    return PostgresMgr::GetInstance()->IsFriend(uid, peer_uid) && PostgresMgr::GetInstance()->IsFriend(peer_uid, uid);
}

bool CallPersistence::LoadUserProfile(int uid, CallUserProfile& profile) const
{
    return PostgresMgr::GetInstance()->GetCallUserProfile(uid, profile);
}

bool CallPersistence::LoadUserProfiles(int uid, int peer_uid, CallUserProfile& user, CallUserProfile& peer) const
{
    return LoadUserProfile(uid, user) && LoadUserProfile(peer_uid, peer);
}

bool CallPersistence::LoadSession(const std::string& call_id, CallSessionInfo& session) const
{
    return PostgresMgr::GetInstance()->GetCallSession(call_id, session);
}

bool CallPersistence::SaveSession(const CallSessionInfo& session) const
{
    return PostgresMgr::GetInstance()->UpsertCallSession(session);
}
