#pragma once

#include "routing/GateRequest.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace memochat::auth
{

// Extracts the access token from an HTTP Authorization header. Only the
// current `Bearer <access_token>` contract is accepted.
std::string ExtractBearerAccessToken(std::string_view authorization);

std::string FindHeaderValueCaseInsensitive(const std::unordered_map<std::string, std::string>& headers,
                                           std::string_view name);

bool ResolveBearerAccessUserId(const memochat::gate::routing::GateRequest& request, int& uid);

} // namespace memochat::auth
