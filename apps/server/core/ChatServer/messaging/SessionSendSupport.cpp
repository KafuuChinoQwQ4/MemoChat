#include "SessionSendSupport.hpp"

#include "CSession.hpp"

void SendChatSessionPayload(const std::shared_ptr<CSession>& session, std::string payload, short msg_id)
{
    if (session)
    {
        session->Send(std::move(payload), msg_id);
    }
}
