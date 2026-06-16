#include "services/account/AccountPersistence.h"

#include "PostgresMgr.h"

namespace memochat::gate::services::account
{

namespace
{

AccountProfile ToAccountProfile(const ::UserInfo& userInfo)
{
    AccountProfile profile;
    profile.name = userInfo.name;
    profile.password = userInfo.pwd;
    profile.uid = userInfo.uid;
    profile.user_id = userInfo.user_id;
    profile.email = userInfo.email;
    profile.nick = userInfo.nick;
    profile.icon = userInfo.icon;
    profile.desc = userInfo.desc;
    profile.sex = userInfo.sex;
    return profile;
}

} // namespace

AccountPersistence& AccountPersistence::Instance()
{
    static AccountPersistence instance;
    return instance;
}

int AccountPersistence::RegisterUser(const std::string& name,
                                     const std::string& email,
                                     const std::string& password,
                                     const std::string& icon)
{
    return PostgresMgr::GetInstance()->RegUser(name, email, password, icon);
}

std::string AccountPersistence::GetUserPublicId(int uid)
{
    return PostgresMgr::GetInstance()->GetUserPublicId(uid);
}

bool AccountPersistence::EmailMatchesUser(const std::string& name, const std::string& email)
{
    return PostgresMgr::GetInstance()->CheckEmail(name, email);
}

bool AccountPersistence::UpdatePassword(const std::string& email, const std::string& password)
{
    return PostgresMgr::GetInstance()->UpdatePwd(email, password);
}

bool AccountPersistence::CheckPassword(const std::string& email, const std::string& password, AccountProfile& profile)
{
    ::UserInfo userInfo;
    if (!PostgresMgr::GetInstance()->CheckPwd(email, password, userInfo))
    {
        return false;
    }

    profile = ToAccountProfile(userInfo);
    return true;
}

} // namespace memochat::gate::services::account
