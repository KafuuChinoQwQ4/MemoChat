#pragma once

#include "ports/IDeliveryGateway.hpp"
#include "ports/IEventPublisher.hpp"
#include "ports/IMessageRepository.hpp"
#include "ports/IOnlineRouteStore.hpp"
#include "ports/IPrivateMessageService.hpp"
#include "ports/IRelationRepository.hpp"
#include "ports/ISessionRegistry.hpp"

#include <memory>
#include <string>

class IChatSession;

class PrivateMessageService : public IPrivateMessageService
{
public:
    PrivateMessageService(ISessionRegistry* session_registry,
                          IOnlineRouteStore* online_route_store,
                          IMessageRepository* message_repository,
                          IRelationRepository* relation_repository,
                          IDeliveryGateway* delivery_gateway,
                          IEventPublisher* event_publisher);

    MessageCommandResult TextChatMessage(const MessageCommandRequest& request) override;
    MessageCommandResult ForwardPrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult PrivateReadAck(const MessageCommandRequest& request) override;
    MessageCommandResult EditPrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult RevokePrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult PrivateHistory(const MessageCommandRequest& request) override;

    void HandleTextChatMessage(const std::shared_ptr<IChatSession>& session,
                               short msg_id,
                               const std::string& msg_data) override;
    void HandleForwardPrivateMessage(const std::shared_ptr<IChatSession>& session,
                                     short msg_id,
                                     const std::string& msg_data) override;
    void HandlePrivateReadAck(const std::shared_ptr<IChatSession>& session,
                              short msg_id,
                              const std::string& msg_data) override;
    void HandleEditPrivateMessage(const std::shared_ptr<IChatSession>& session,
                                  short msg_id,
                                  const std::string& msg_data) override;
    void HandleRevokePrivateMessage(const std::shared_ptr<IChatSession>& session,
                                    short msg_id,
                                    const std::string& msg_data) override;
    void HandlePrivateHistory(const std::shared_ptr<IChatSession>& session,
                              short msg_id,
                              const std::string& msg_data) override;

private:
    ISessionRegistry* _session_registry = nullptr;
    IOnlineRouteStore* _online_route_store = nullptr;
    IMessageRepository* _message_repository = nullptr;
    IRelationRepository* _relation_repository = nullptr;
    IDeliveryGateway* _delivery_gateway = nullptr;
    IEventPublisher* _event_publisher = nullptr;
};
