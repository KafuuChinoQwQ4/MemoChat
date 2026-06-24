#include "MessageServiceFactory.h"

#include "MessageGrpcServiceAdapter.h"
#include "GroupMessageService.h"
#include "PrivateMessageService.h"
#include "logging/Logger.h"

#include <stdexcept>

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
                                                                    IEventPublisher* event_publisher)
{
    const auto backend = message_service_config.MessageServiceBackend();
    if (backend == "inprocess")
    {
        return CreateInProcessPrivateMessageService(session_registry,
                                                    online_route_store,
                                                    message_repository,
                                                    relation_repository,
                                                    delivery_gateway,
                                                    event_publisher);
    }
    if (backend == "grpc" || backend == "remote")
    {
        const auto endpoint = message_service_config.MessageServiceEndpoint();
        if (endpoint.empty())
        {
            throw std::runtime_error("Message service remote endpoint is empty: " + backend);
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
                                                                IEventPublisher* event_publisher)
{
    const auto backend = message_service_config.MessageServiceBackend();
    if (backend == "inprocess")
    {
        return CreateInProcessGroupMessageService(message_repository,
                                                  relation_repository,
                                                  delivery_gateway,
                                                  event_publisher);
    }
    if (backend == "grpc" || backend == "remote")
    {
        const auto endpoint = message_service_config.MessageServiceEndpoint();
        if (endpoint.empty())
        {
            throw std::runtime_error("Message service remote endpoint is empty: " + backend);
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
