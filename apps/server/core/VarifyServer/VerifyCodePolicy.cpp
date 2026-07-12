#include "VerifyCodePolicy.hpp"

#include <algorithm>
#include <sodium.h>
#include <string_view>

import memochat.varify.verify_code_algorithms;

namespace varifyservice
{

std::string BuildVerifyCodeKey(const std::string& email)
{
    return std::string(kVerifyCodePrefix) + email;
}

bool IsSyntheticLoadtestEmail(const std::string& email)
{
    constexpr std::string_view suffix = "@loadtest.local";
    return memochat::varify::modules::EndsWithAsciiCaseInsensitive(email.data(),
                                                                   email.size(),
                                                                   suffix.data(),
                                                                   suffix.size());
}

bool IsValidVerifyEmail(const std::string& email)
{
    const auto at = email.find('@');
    if (at == std::string::npos || at == 0 || email.find('@', at + 1) != std::string::npos)
    {
        return false;
    }
    const auto is_ascii_alpha = [](unsigned char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    };
    const auto is_ascii_alnum = [&](unsigned char c)
    {
        return is_ascii_alpha(c) || (c >= '0' && c <= '9');
    };
    const auto is_local_char = [&](unsigned char c)
    {
        return is_ascii_alnum(c) || c == '.' || c == '_' || c == '%' || c == '+' || c == '-';
    };
    if (!std::all_of(email.begin(), email.begin() + static_cast<std::string::difference_type>(at), is_local_char))
    {
        return false;
    }

    const std::string_view domain(email.data() + at + 1, email.size() - at - 1);
    const auto dot = domain.rfind('.');
    if (dot == std::string_view::npos || dot == 0)
    {
        return false;
    }
    const std::string_view suffix = domain.substr(dot + 1);
    if (suffix.size() < 2 || suffix.size() > 63)
    {
        return false;
    }
    if (!std::all_of(domain.begin(),
                     domain.begin() + static_cast<std::string_view::difference_type>(dot),
                     [&](unsigned char c)
                     {
                         return is_ascii_alnum(c) || c == '.' || c == '-';
                     }))
    {
        return false;
    }
    return std::all_of(suffix.begin(),
                       suffix.end(),
                       [&](unsigned char c)
                       {
                           return is_ascii_alpha(c);
                       });
}

std::string GenerateNumericVerifyCode(int length)
{
    length =
        memochat::varify::modules::NormalizeVerifyCodeLength(length, kDefaultVerifyCodeLength, kMaxVerifyCodeLength);

    static const bool sodium_ready = sodium_init() >= 0;
    if (!sodium_ready)
    {
        return {};
    }

    std::string code;
    code.reserve(static_cast<std::size_t>(length));
    for (int i = 0; i < length; ++i)
    {
        code.push_back(static_cast<char>('0' + randombytes_uniform(10)));
    }
    return code;
}

} // namespace varifyservice
