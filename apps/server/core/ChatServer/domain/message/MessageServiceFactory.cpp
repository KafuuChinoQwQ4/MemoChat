#include "MessageServiceFactory.hpp"

#include "MessageGrpcServiceAdapter.hpp"
#include "GroupMessageService.hpp"
#include "PrivateMessageService.hpp"
#include "logging/Logger.hpp"

import memochat.chat.service_factory_algorithms;

std::unique_ptr<IPrivateMessageService> CreateInProcessPrivateMessageService(ISessionRegistry* session_registry,
                                                                             IOnlineRouteStore* online_route_store,
                                                                             IMessageRepository* message_repository,
                                                                             IRelationRepository* relation_repository,
                                                                             IDeliveryGateway* delivery_gateway,
                                                                             IEventPublisher* event_publisher)
{
    return std::make_unique<PrivateMessageService>(session_registry,
                                                   online_route_store,
                                                   message_repository,
                                                   relation_repository,
                                                   delivery_gateway,
                                                   event_publisher);
}

std::unique_ptr<IGroupMessageService> CreateInProcessGroupMessageService(IMessageRepository* message_repository,
                                                                         IRelationRepository* relation_repository,
                                                                         IDeliveryGateway* delivery_gateway,
                                                                         IEventPublisher* event_publisher)
{
    return std::make_unique<GroupMessageService>(message_repository,
                                                 relation_repository,
                                                 delivery_gateway,
                                                 event_publisher);
}

std::unique_ptr<IPrivateMessageService> CreatePrivateMessageService(const IMessageServiceConfig& message_service_config,
                                                                    ISessionRegistry* session_registry,
                                                                    IOnlineRouteStore* online_route_store,
                                                                    IMessageRepository* message_repository,
                                                                    IRelationRepository* relation_repository,
                                                                    IDeliveryGateway* delivery_gateway,
                                                                    IEventPublisher* event_publisher,
                                                                    std::string* error)
{
    const auto backend = message_service_config.MessageServiceBackend();
    if (memochat::chat::factory::modules::IsInProcessBackend(backend.data(), backend.size()))
    {
        return CreateInProcessPrivateMessageService(session_registry,
                                                    online_route_store,
                                                    message_repository,
                                                    relation_repository,
                                                    delivery_gateway,
                                                    event_publisher);
    }
    if (memochat::chat::factory::modules::IsRemoteBackend(backend.data(), backend.size()))
    {
        const auto endpoint = message_service_config.MessageServiceEndpoint();
        if (endpoint.empty())
        {
            const std::string message = "Message service remote endpoint is empty: " + backend;
            if (error != nullptr)
            {
                *error = message;
            }
            memolog::LogError("chat.message_service.endpoint_missing", message, {{"configured_backend", backend}});
            return nullptr;
        }
        return std::make_unique<MessageGrpcServiceAdapter>(endpoint);
    }

    memolog::LogWarn("chat.message_service.unsupported_backend",
                     "message service backend is not implemented yet, fallback to inprocess",
                     {{"configured_backend", backend}, {"fallback_backend", "inprocess"}});
    return CreateInProcessPrivateMessageService(session_registry,
                                                online_route_store,
                                                message_repository,
                                                relation_repository,
                                                delivery_gateway,
                                                event_publisher);
}

std::unique_ptr<IGroupMessageService> CreateGroupMessageService(const IMessageServiceConfig& message_service_config,
                                                                IMessageRepository* message_repository,
                                                                IRelationRepository* relation_repository,
                                                                IDeliveryGateway* delivery_gateway,
                                                                IEventPublisher* event_publisher,
                                                                std::string* error)
{
    const auto backend = message_service_config.MessageServiceBackend();
    if (memochat::chat::factory::modules::IsInProcessBackend(backend.data(), backend.size()))
    {
        return CreateInProcessGroupMessageService(message_repository,
                                                  relation_repository,
                                                  delivery_gateway,
                                                  event_publisher);
    }
    if (memochat::chat::factory::modules::IsRemoteBackend(backend.data(), backend.size()))
    {
        const auto endpoint = message_service_config.MessageServiceEndpoint();
        if (endpoint.empty())
        {
            const std::string message = "Message service remote endpoint is empty: " + backend;
            if (error != nullptr)
            {
                *error = message;
            }
            memolog::LogError("chat.message_service.endpoint_missing", message, {{"configured_backend", backend}});
            return nullptr;
        }
        return std::make_unique<MessageGrpcServiceAdapter>(endpoint);
    }

    memolog::LogWarn("chat.message_service.unsupported_backend",
                     "message service backend is not implemented yet, fallback to inprocess",
                     {{"configured_backend", backend}, {"fallback_backend", "inprocess"}});
    return CreateInProcessGroupMessageService(message_repository,
                                              relation_repository,
                                              delivery_gateway,
                                              event_publisher);
}
