#include "ChatAccountDirectory.hpp"

#include "PostgresMgr.hpp"

std::shared_ptr<UserInfo> ChatAccountDirectory::GetByUid(int uid)
{
    return PostgresMgr::GetInstance()->GetUser(uid);
}

std::shared_ptr<UserInfo> ChatAccountDirectory::GetByName(const std::string& name)
{
    return PostgresMgr::GetInstance()->GetUser(name);
}

bool ChatAccountDirectory::ResolveUidByPublicId(const std::string& user_id, int& uid)
{
    return PostgresMgr::GetInstance()->GetUidByUserId(user_id, uid);
}

std::unordered_map<int, std::shared_ptr<UserInfo>> ChatAccountDirectory::GetManyByUids(const std::vector<int>& uids)
{
    return PostgresMgr::GetInstance()->GetUsersByUids(uids);
}
