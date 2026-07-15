#pragma once

#include "Singleton.hpp"
#include "ports/IAccountDirectory.hpp"

// Default IAccountDirectory adapter for ChatServer. Process-wide Singleton so
// free-function / transport / client call sites (ChatUserSupport, ChatServiceImpl,
// ChatGrpcClient) can reach the seam without a full DI rewrite; DI-aware callers
// (ChatRelationRepository) use the same instance.
//
// Backed today by PostgresMgr's account methods, which open the [AccountPostgres]
// bridge to memo_account. Replacing this adapter is the only change needed for a
// later gRPC snapshot API or event-projection backend.
class ChatAccountDirectory
    : public Singleton<ChatAccountDirectory>
    , public IAccountDirectory
{
    friend class Singleton<ChatAccountDirectory>;

public:
    std::shared_ptr<UserInfo> GetByUid(int uid) override;
    std::shared_ptr<UserInfo> GetByName(const std::string& name) override;
    bool ResolveUidByPublicId(const std::string& user_id, int& uid) override;
    std::unordered_map<int, std::shared_ptr<UserInfo>> GetManyByUids(const std::vector<int>& uids) override;

private:
    ChatAccountDirectory() = default;
};

// Convenience accessor — every ChatServer account-data read should go through
// this (or an injected IAccountDirectory*) rather than PostgresMgr.
inline IAccountDirectory& AccountDirectory()
{
    return *ChatAccountDirectory::GetInstance();
}
