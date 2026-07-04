#pragma once

#include <string>

namespace gateauthsupport
{

struct UserInfo
{
    std::string name;
    std::string pwd;
    int uid = 0;
    std::string user_id;
    std::string email;
    std::string nick;
    std::string icon;
    std::string desc;
    int sex = 0;
};

} // namespace gateauthsupport
