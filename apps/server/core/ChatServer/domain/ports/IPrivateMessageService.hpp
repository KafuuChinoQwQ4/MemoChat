#pragma once

#include "ports/IMessageCommandService.hpp"

#include <memory>
#include <string>

class IChatSession;

class IPrivateMessageService : public IPrivateMessageCommandService
{
public:
    virtual ~IPrivateMessageService() = default;

    virtual void
    HandleTextChatMessage(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void HandleForwardPrivateMessage(const std::shared_ptr<IChatSession>& session,
                                             short msg_id,
                                             const std::string& msg_data) = 0;
    virtual void
    HandlePrivateReadAck(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void HandleEditPrivateMessage(const std::shared_ptr<IChatSession>& session,
                                          short msg_id,
                                          const std::string& msg_data) = 0;
    virtual void HandleRevokePrivateMessage(const std::shared_ptr<IChatSession>& session,
                                            short msg_id,
                                            const std::string& msg_data) = 0;
    virtual void
    HandlePrivateHistory(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
};
