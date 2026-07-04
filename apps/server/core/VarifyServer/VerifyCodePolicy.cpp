#include "VerifyCodePolicy.hpp"

#include <algorithm>
#include <cctype>
#include <random>
#include <regex>
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
    static const std::regex kEmailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,63})");
    return std::regex_match(email, kEmailRegex);
}

std::string GenerateNumericVerifyCode(int length)
{
    length =
        memochat::varify::modules::NormalizeVerifyCodeLength(length, kDefaultVerifyCodeLength, kMaxVerifyCodeLength);

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> digit_dist(0, 9);

    std::string code;
    code.reserve(static_cast<std::size_t>(length));
    for (int i = 0; i < length; ++i)
    {
        code.push_back(static_cast<char>('0' + digit_dist(rng)));
    }
    return code;
}

} // namespace varifyservice
