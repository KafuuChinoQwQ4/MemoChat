#include "support/UserTokenValidator.hpp"

#include "RedisMgr.hpp"
#include "const.hpp"

#include <string>

import memochat.gate.user_token_validator_algorithms;

namespace memochat::auth
{
bool ValidateUserToken(int uid, const std::string& token)
{
    if (!memochat::gate::auth::modules::ShouldValidateUserToken(uid, token.empty()))
    {
        return false;
    }
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string token_value;
    const bool redis_hit = RedisMgr::GetInstance()->Get(token_key, token_value);
    return memochat::gate::auth::modules::ShouldAcceptStoredUserToken(redis_hit,
                                                                      token_value.empty(),
                                                                      token_value == token);
}
} // namespace memochat::auth
