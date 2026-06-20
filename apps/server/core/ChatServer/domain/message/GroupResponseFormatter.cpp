#include "GroupResponseFormatter.h"

#include "ChatHistoryOutputDtos.h"

namespace memochat::chat::message::GroupResponseFormatter
{

void ApplyPermissionFlags(memochat::json::JsonValue& out, int64_t permission_bits)
{
    memochat::chat::output::AppendGroupPermissionFlags(out, permission_bits);
}

memochat::json::JsonValue SerializeHistoryMessage(const GroupMessageInfo& msg)
{
    return memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatGroupHistoryMessageFromInfo(msg));
}

} // namespace memochat::chat::message::GroupResponseFormatter
