#pragma once

#include "CallSessionTypes.hpp"

#include <string>

class CallPersistence
{
public:
    static CallPersistence& Instance();

    bool AreUsersMutualFriends(int uid, int peer_uid) const;
    bool LoadUserProfile(int uid, CallUserProfile& profile) const;
    bool LoadUserProfiles(int uid, int peer_uid, CallUserProfile& user, CallUserProfile& peer) const;
    bool LoadSession(const std::string& call_id, CallSessionInfo& session) const;
    bool SaveSession(const CallSessionInfo& session) const;

private:
    CallPersistence() = default;
};
