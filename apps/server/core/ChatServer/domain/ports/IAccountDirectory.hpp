#pragma once

#include "data.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Process-wide port for ChatServer's account-domain reads (finding #3 isolation
// seam). All ChatServer code that needs user identity / profile fields must go
// through this interface — not through PostgresMgr account methods or raw SQL
// against the [AccountPostgres] bridge.
//
// Today's adapter (ChatAccountDirectory) still backs the port with the existing
// [AccountPostgres] → memo_account bridge, so behavior is unchanged. The port is
// the common prerequisite for later swapping the adapter to a gRPC snapshot API
// or an event-synced local projection without touching callers.
//
// Minimal projection: uid, user_id, name, nick, icon, sex, desc, email.
class IAccountDirectory
{
public:
    virtual ~IAccountDirectory() = default;

    // Lookup by internal uid. Returns nullptr when the user does not exist.
    virtual std::shared_ptr<UserInfo> GetByUid(int uid) = 0;

    // Lookup by login/handle name. Returns nullptr when the user does not exist.
    virtual std::shared_ptr<UserInfo> GetByName(const std::string& name) = 0;

    // Resolve the public user_id (snowflake) to the internal uid.
    virtual bool ResolveUidByPublicId(const std::string& user_id, int& uid) = 0;

    // Batch lookup by uid set. Missing uids are simply absent from the map.
    // Used by relation/group/message enrichment that previously JOIN-ed "user".
    virtual std::unordered_map<int, std::shared_ptr<UserInfo>> GetManyByUids(const std::vector<int>& uids) = 0;
};
