#pragma once

#include <chrono>
#include <cstdint>
#include <string>

// Shared helpers for the ChatServer Postgres DAO translation units.
// Defined `inline` (not in an anonymous namespace) so the single definition is
// shared across PostgresDaoGroups.cpp / PostgresDaoDialogs.cpp / PostgresDaoPrivateMessages.cpp
// without ODR violations or per-TU duplication.

// Group permission bits. Identical values were previously duplicated in
// PostgresDaoGroups.cpp and PostgresDaoDialogs.cpp.
inline constexpr int64_t kPermChangeGroupInfo = 1LL << 0;
inline constexpr int64_t kPermDeleteMessages = 1LL << 1;
inline constexpr int64_t kPermInviteUsers = 1LL << 2;
inline constexpr int64_t kPermManageAdmins = 1LL << 3;
inline constexpr int64_t kPermPinMessages = 1LL << 4;
inline constexpr int64_t kPermBanUsers = 1LL << 5;
inline constexpr int64_t kPermManageTopics = 1LL << 6;
inline constexpr int64_t kDefaultAdminPermBits =
    kPermChangeGroupInfo | kPermDeleteMessages | kPermInviteUsers | kPermPinMessages | kPermBanUsers;
inline constexpr int64_t kOwnerPermBits = kDefaultAdminPermBits | kPermManageAdmins | kPermManageTopics;

// Wall-clock milliseconds since epoch, used by the DAO layer.
inline int64_t NowMs()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

// Conversation preview text derived from a message type/content pair.
// Body is identical to the previous PostgresDaoGroups.cpp / PostgresDaoDialogs.cpp copies.
inline std::string BuildPreviewText(const std::string& msg_type, const std::string& content)
{
    if (msg_type == "image" || content.rfind("__memochat_img__:", 0) == 0)
    {
        return "[图片]";
    }
    if (msg_type == "file" || content.rfind("__memochat_file__:", 0) == 0)
    {
        return "[文件]";
    }
    if (msg_type == "call" || content.rfind("__memochat_call__:", 0) == 0)
    {
        return "[通话邀请]";
    }
    if (content.rfind("__memochat_reply__:", 0) == 0)
    {
        return "[回复]";
    }
    if (content.size() <= 80)
    {
        return content;
    }
    return content.substr(0, 80);
}
