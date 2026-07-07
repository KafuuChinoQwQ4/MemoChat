#include "support/BearerAccessAuth.hpp"

#include "support/UserTokenValidator.hpp"

import memochat.gate.bearer_access_auth_algorithms;

namespace memochat::auth
{

std::string ExtractBearerAccessToken(std::string_view authorization)
{
    unsigned long long token_offset = 0;
    unsigned long long token_size = 0;
    if (!memochat::gate::auth::bearer_modules::TryLocateBearerToken(authorization.data(),
                                                                    authorization.size(),
                                                                    token_offset,
                                                                    token_size))
    {
        return "";
    }
    return std::string(
        authorization.substr(static_cast<std::size_t>(token_offset), static_cast<std::size_t>(token_size)));
}

std::string FindHeaderValueCaseInsensitive(const std::unordered_map<std::string, std::string>& headers,
                                           std::string_view name)
{
    for (const auto& [key, value] : headers)
    {
        if (memochat::gate::auth::bearer_modules::HeaderNameEquals(key.data(), key.size(), name.data(), name.size()))
        {
            return value;
        }
    }
    return "";
}

bool ResolveBearerAccessUserId(const memochat::gate::routing::GateRequest& request, int& uid)
{
    uid = 0;
    const std::string token =
        ExtractBearerAccessToken(FindHeaderValueCaseInsensitive(request.headers, "authorization"));
    return !token.empty() && ResolveUserIdFromToken(token, uid);
}

} // namespace memochat::auth
