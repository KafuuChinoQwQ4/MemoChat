#pragma once

#include <string>

namespace varifyservice
{

constexpr const char* kVerifyCodePrefix = "code_";
constexpr int kDefaultVerifyCodeLength = 6;
constexpr int kMaxVerifyCodeLength = 12;

std::string BuildVerifyCodeKey(const std::string& email);
bool IsSyntheticLoadtestEmail(const std::string& email);
bool IsValidVerifyEmail(const std::string& email);
std::string GenerateNumericVerifyCode(int length);

} // namespace varifyservice
