#pragma once

#include "const.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gateauthsupport {

struct UserInfo {
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

struct ChatRouteNode {
    std::string name;
    std::string host;
    std::string port;
    std::string quic_host;
    std::string quic_port;
    int online_count = 0;
    int priority = 0;
};

const char* MinClientVersion();
int LoginProtocolVersion();
int64_t NowMs();
bool IsQuicRolloutEnabled();
bool IsClientVersionAllowed(const std::string& clientVer, const std::string& minVer);
std::string GetChatAuthSecret();
int GetChatTicketTtlSec();
int GetLoginCacheTtlSec();
bool TryLoadCachedLoginProfile(const std::string& email, const std::string& pwd, UserInfo& userInfo);
void CacheLoginProfile(const std::string& email, const UserInfo& userInfo);
void InvalidateLoginCacheByEmail(const std::string& email);
void InvalidateLoginCacheByUid(int uid);
std::vector<ChatRouteNode> LoadGateChatRouteNodes(std::vector<std::string>* load_snapshot = nullptr,
                                                  std::vector<std::string>* least_loaded_snapshot = nullptr);

} // namespace gateauthsupport
