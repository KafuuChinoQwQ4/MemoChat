#include "SessionSendSupport.hpp"

#include "IChatSession.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"

#include <utility>

namespace
{
std::string JsonToWireStringSessionSend(const memochat::json::JsonValue& value)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, value);
}
} // namespace

void SendChatSessionPayload(const std::shared_ptr<IChatSession>& session, std::string payload, short msg_id)
{
    if (session)
    {
        session->send(std::move(payload), msg_id);
    }
}

void SendChatSessionOfflineNotification(const std::shared_ptr<IChatSession>& session, int uid)
{
    if (!session)
    {
        return;
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;

    session->send(JsonToWireStringSessionSend(rtvalue), ID_NOTIFY_OFF_LINE_REQ);
}
