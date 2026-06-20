#pragma once

#include "json/GlazeCompat.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace memochat::chat::relation
{

struct ChatSearchUserRequestDto
{
    std::string user_id;
};

struct ChatSearchUserResponseDto
{
    int error = 0;
    std::optional<int> uid;
    std::optional<std::string> user_id;
    std::optional<std::string> name;
    std::optional<std::string> email;
    std::optional<std::string> nick;
    std::optional<std::string> desc;
    std::optional<int> sex;
    std::optional<std::string> icon;
};

struct ChatFilterFriendUidsRequestDto
{
    int viewer_uid = 0;
    std::vector<int> author_uids;
};

struct ChatFilterFriendUidsResponseDto
{
    int error = 0;
    std::vector<int> friend_uids;
};

struct ChatAddFriendApplyRequestDto
{
    int uid = 0;
    std::string applyname;
    int touid = 0;
    std::vector<std::string> labels;
};

struct ChatAddFriendApplyResponseDto
{
    int error = 0;
};

struct ChatAuthFriendApplyRequestDto
{
    int fromuid = 0;
    int touid = 0;
    std::string back;
    std::vector<std::string> labels;
};

struct ChatAuthFriendApplyResponseDto
{
    int error = 0;
    std::optional<std::string> name;
    std::optional<std::string> nick;
    std::optional<std::string> icon;
    std::optional<int> sex;
    std::optional<int> uid;
    std::optional<std::string> user_id;
};

struct ChatDeleteFriendRequestDto
{
    int uid = 0;
    int friend_uid = 0;
};

struct ChatDeleteFriendResponseDto
{
    int error = 0;
    int fromuid = 0;
    int friend_uid = 0;
};

struct ChatDialogListRequestDto
{
    int uid = 0;
};

struct ChatDialogListResponseDto
{
    int error = 0;
    int uid = 0;
    std::optional<memochat::json::JsonValue> dialogs;
};

struct ChatSyncDraftRequestDto
{
    int uid = 0;
    std::string dialog_type;
    int peer_uid = 0;
    int64_t group_id = 0;
    std::string draft_text;
    bool has_mute_state = false;
    int mute_state = 0;
};

struct ChatSyncDraftResponseDto
{
    int error = 0;
    int uid = 0;
    std::string dialog_type;
    int peer_uid = 0;
    std::string draft_text;
    std::optional<int> mute_state;
    std::optional<int64_t> group_id;
};

struct ChatPinDialogRequestDto
{
    int uid = 0;
    std::string dialog_type;
    int peer_uid = 0;
    int64_t group_id = 0;
    int pinned_rank = 0;
};

struct ChatPinDialogResponseDto
{
    int error = 0;
    int uid = 0;
    std::string dialog_type;
    int peer_uid = 0;
    int pinned_rank = 0;
    std::optional<int64_t> group_id;
};

struct ChatAddFriendApplyNotifyDto
{
    int error = 0;
    int applyuid = 0;
    std::string name;
    std::string desc;
    std::optional<std::string> icon;
    std::optional<int> sex;
    std::optional<std::string> nick;
    std::optional<std::string> user_id;
};

struct ChatAuthFriendApplyNotifyDto
{
    int error = 0;
    int fromuid = 0;
    int touid = 0;
    std::optional<std::string> name;
    std::optional<std::string> nick;
    std::optional<std::string> icon;
    std::optional<int> sex;
    std::optional<std::string> user_id;
};

struct ChatRelationStateEventDto
{
    std::string event_type;
    int uid = 0;
    int touid = 0;
    std::vector<int> uids;
    int64_t event_ts = 0;
    std::vector<std::string> labels;
};

inline int ReadFromUidOrUid(const memochat::json::JsonValue& root)
{
    return root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
}

inline int64_t ReadGroupIdAlias(const memochat::json::JsonValue& root)
{
    return root.isMember("groupid") ? root["groupid"].asInt64() : root.get("group_id", 0).asInt64();
}

inline std::vector<std::string> ReadLabelsArray(const memochat::json::JsonValue& root)
{
    std::vector<std::string> labels;
    if (root.isMember("labels") && root["labels"].isArray())
    {
        for (const auto& item : root["labels"])
        {
            labels.push_back(item.asString());
        }
    }
    return labels;
}

inline ChatSearchUserRequestDto ChatSearchUserRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatSearchUserRequestDto request;
    request.user_id = root["user_id"].asString();
    return request;
}

inline ChatSearchUserResponseDto ChatSearchUserResponseFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatSearchUserResponseDto response;
    response.error = root.get("error", 0).asInt();
    if (root.isMember("uid"))
    {
        response.uid = root["uid"].asInt();
    }
    if (root.isMember("user_id"))
    {
        response.user_id = root["user_id"].asString();
    }
    if (root.isMember("name"))
    {
        response.name = root["name"].asString();
    }
    if (root.isMember("email"))
    {
        response.email = root["email"].asString();
    }
    if (root.isMember("nick"))
    {
        response.nick = root["nick"].asString();
    }
    if (root.isMember("desc"))
    {
        response.desc = root["desc"].asString();
    }
    if (root.isMember("sex"))
    {
        response.sex = root["sex"].asInt();
    }
    if (root.isMember("icon"))
    {
        response.icon = root["icon"].asString();
    }
    return response;
}

inline ChatFilterFriendUidsRequestDto ChatFilterFriendUidsRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatFilterFriendUidsRequestDto request;
    request.viewer_uid = root.isMember("viewer_uid") ? root["viewer_uid"].asInt() : 0;
    if (root.isMember("author_uids") && root["author_uids"].isArray())
    {
        for (const auto& item : root["author_uids"])
        {
            const int author_uid = item.asInt();
            if (author_uid > 0)
            {
                request.author_uids.push_back(author_uid);
            }
        }
    }
    return request;
}

inline ChatAddFriendApplyRequestDto ChatAddFriendApplyRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatAddFriendApplyRequestDto request;
    request.uid = root["uid"].asInt();
    request.applyname = root["applyname"].asString();
    request.touid = root["touid"].asInt();
    request.labels = ReadLabelsArray(root);
    return request;
}

inline ChatAuthFriendApplyRequestDto ChatAuthFriendApplyRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatAuthFriendApplyRequestDto request;
    request.fromuid = root["fromuid"].asInt();
    request.touid = root["touid"].asInt();
    request.back = root["back"].asString();
    request.labels = ReadLabelsArray(root);
    return request;
}

inline ChatDeleteFriendRequestDto ChatDeleteFriendRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatDeleteFriendRequestDto request;
    request.uid = ReadFromUidOrUid(root);
    request.friend_uid = root.isMember("friend_uid") ? root["friend_uid"].asInt() : root["touid"].asInt();
    return request;
}

inline ChatDialogListRequestDto ChatDialogListRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatDialogListRequestDto request;
    request.uid = ReadFromUidOrUid(root);
    return request;
}

inline ChatSyncDraftRequestDto ChatSyncDraftRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatSyncDraftRequestDto request;
    request.uid = ReadFromUidOrUid(root);
    request.dialog_type = root.get("dialog_type", "").asString();
    request.peer_uid = root.get("peer_uid", 0).asInt();
    request.group_id = ReadGroupIdAlias(root);
    request.draft_text = root.get("draft_text", "").asString();
    if (request.draft_text.size() > 2000)
    {
        request.draft_text.resize(2000);
    }
    request.has_mute_state = root.isMember("mute_state");
    request.mute_state = root.get("mute_state", 0).asInt();
    return request;
}

inline ChatPinDialogRequestDto ChatPinDialogRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ChatPinDialogRequestDto request;
    request.uid = ReadFromUidOrUid(root);
    request.dialog_type = root.get("dialog_type", "").asString();
    request.peer_uid = root.get("peer_uid", 0).asInt();
    request.group_id = ReadGroupIdAlias(root);
    request.pinned_rank = std::clamp(root.get("pinned_rank", 0).asInt(), 0, 1000000000);
    return request;
}

inline memochat::json::JsonValue ToJsonValue(const ChatSearchUserResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    if (dto.uid.has_value())
    {
        value["uid"] = *dto.uid;
    }
    if (dto.user_id.has_value())
    {
        value["user_id"] = *dto.user_id;
    }
    if (dto.name.has_value())
    {
        value["name"] = *dto.name;
    }
    if (dto.email.has_value())
    {
        value["email"] = *dto.email;
    }
    if (dto.nick.has_value())
    {
        value["nick"] = *dto.nick;
    }
    if (dto.desc.has_value())
    {
        value["desc"] = *dto.desc;
    }
    if (dto.sex.has_value())
    {
        value["sex"] = *dto.sex;
    }
    if (dto.icon.has_value())
    {
        value["icon"] = *dto.icon;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatFilterFriendUidsResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["friend_uids"] = memochat::json::array_t{};
    for (const int friend_uid : dto.friend_uids)
    {
        value["friend_uids"].append(friend_uid);
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatDeleteFriendResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["fromuid"] = dto.fromuid;
    value["friend_uid"] = dto.friend_uid;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatDialogListResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["uid"] = dto.uid;
    if (dto.dialogs.has_value())
    {
        value["dialogs"] = *dto.dialogs;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatSyncDraftResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["uid"] = dto.uid;
    value["dialog_type"] = dto.dialog_type;
    value["peer_uid"] = dto.peer_uid;
    value["draft_text"] = dto.draft_text;
    if (dto.mute_state.has_value())
    {
        value["mute_state"] = *dto.mute_state;
    }
    if (dto.group_id.has_value())
    {
        value["group_id"] = static_cast<int64_t>(*dto.group_id);
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatPinDialogResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["uid"] = dto.uid;
    value["dialog_type"] = dto.dialog_type;
    value["peer_uid"] = dto.peer_uid;
    value["pinned_rank"] = dto.pinned_rank;
    if (dto.group_id.has_value())
    {
        value["group_id"] = static_cast<int64_t>(*dto.group_id);
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatAddFriendApplyResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatAuthFriendApplyResponseDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    if (dto.name.has_value())
    {
        value["name"] = *dto.name;
    }
    if (dto.nick.has_value())
    {
        value["nick"] = *dto.nick;
    }
    if (dto.icon.has_value())
    {
        value["icon"] = *dto.icon;
    }
    if (dto.sex.has_value())
    {
        value["sex"] = *dto.sex;
    }
    if (dto.uid.has_value())
    {
        value["uid"] = *dto.uid;
    }
    if (dto.user_id.has_value())
    {
        value["user_id"] = *dto.user_id;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatAddFriendApplyNotifyDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["applyuid"] = dto.applyuid;
    value["name"] = dto.name;
    value["desc"] = dto.desc;
    if (dto.icon.has_value())
    {
        value["icon"] = *dto.icon;
    }
    if (dto.sex.has_value())
    {
        value["sex"] = *dto.sex;
    }
    if (dto.nick.has_value())
    {
        value["nick"] = *dto.nick;
    }
    if (dto.user_id.has_value())
    {
        value["user_id"] = *dto.user_id;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatAuthFriendApplyNotifyDto& dto)
{
    memochat::json::JsonValue value;
    value["error"] = dto.error;
    value["fromuid"] = dto.fromuid;
    value["touid"] = dto.touid;
    if (dto.name.has_value())
    {
        value["name"] = *dto.name;
    }
    if (dto.nick.has_value())
    {
        value["nick"] = *dto.nick;
    }
    if (dto.icon.has_value())
    {
        value["icon"] = *dto.icon;
    }
    if (dto.sex.has_value())
    {
        value["sex"] = *dto.sex;
    }
    if (dto.user_id.has_value())
    {
        value["user_id"] = *dto.user_id;
    }
    return value;
}

inline memochat::json::JsonValue ToJsonValue(const ChatRelationStateEventDto& dto)
{
    memochat::json::JsonValue value;
    value["event_type"] = dto.event_type;
    value["uid"] = dto.uid;
    value["touid"] = dto.touid;
    value["uids"] = memochat::json::array_t{};
    for (const int uid : dto.uids)
    {
        value["uids"].append(uid);
    }
    value["event_ts"] = static_cast<int64_t>(dto.event_ts);
    if (!dto.labels.empty())
    {
        value["labels"] = memochat::json::array_t{};
        for (const auto& label : dto.labels)
        {
            value["labels"].append(label);
        }
    }
    return value;
}

} // namespace memochat::chat::relation
