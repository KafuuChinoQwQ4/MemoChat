#pragma once

#include <memory>
#include <string>
#include <vector>
#include "json/GlazeCompat.h"

struct ApplyInfo;
struct UserInfo;
namespace chatusersupport {

bool IsPureDigit(const std::string& str);
void GetUserByUid(const std::string& uid_str, memochat::json::JsonValue& rtvalue);
void GetUserByName(const std::string& name, memochat::json::JsonValue& rtvalue);
bool GetBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list);

} // namespace chatusersupport
