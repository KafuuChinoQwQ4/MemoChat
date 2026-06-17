#include "support/UserTokenValidator.h"

#include "RedisMgr.h"
#include "const.h"

#include <string>

namespace memochat::auth
{
bool ValidateUserToken(int uid, const std::string& token)
{
    if (uid <= 0 || token.empty())
    {
        return false;
    }
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string token_value;
    if (!RedisMgr::GetInstance()->Get(token_key, token_value))
    {
        return false;
    }
    return !token_value.empty() && token_value == token;
}
} // namespace memochat::auth
