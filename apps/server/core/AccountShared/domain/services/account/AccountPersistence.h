#pragma once

#include <string>

namespace memochat::gate::services::account
{

struct AccountProfile
{
    std::string name;
    std::string password;
    int uid = 0;
    std::string user_id;
    std::string email;
    std::string nick;
    std::string icon;
    std::string desc;
    int sex = 0;
};

class AccountPersistence
{
public:
    static AccountPersistence& Instance();

    int RegisterUser(const std::string& name,
                     const std::string& email,
                     const std::string& password,
                     const std::string& icon);
    std::string GetUserPublicId(int uid);
    bool EmailMatchesUser(const std::string& name, const std::string& email);
    bool UpdatePassword(const std::string& email, const std::string& password);
    bool CheckPassword(const std::string& email, const std::string& password, AccountProfile& profile);

private:
    AccountPersistence() = default;
};

} // namespace memochat::gate::services::account
