#pragma once

#include "ports/IDeliveryGateway.hpp"
#include "ports/IEventPublisher.hpp"
#include "ports/IGroupMessageService.hpp"
#include "ports/IMessageRepository.hpp"
#include "ports/IMessageServiceConfig.hpp"
#include "ports/IOnlineRouteStore.hpp"
#include "ports/IPrivateMessageService.hpp"
#include "ports/IRelationRepository.hpp"
#include "ports/ISessionRegistry.hpp"

#include <memory>

std::unique_ptr<IPrivateMessageService> CreateInProcessPrivateMessageService(ISessionRegistry* session_registry,
                                                                             IOnlineRouteStore* online_route_store,
                                                                             IMessageRepository* message_repository,
                                                                             IRelationRepository* relation_repository,
                                                                             IDeliveryGateway* delivery_gateway,
                                                                             IEventPublisher* event_publisher);

std::unique_ptr<IGroupMessageService> CreateInProcessGroupMessageService(IMessageRepository* message_repository,
                                                                         IRelationRepository* relation_repository,
                                                                         IDeliveryGateway* delivery_gateway,
                                                                         IEventPublisher* event_publisher);

std::unique_ptr<IPrivateMessageService> CreatePrivateMessageService(const IMessageServiceConfig& message_service_config,
                                                                    ISessionRegistry* session_registry,
                                                                    IOnlineRouteStore* online_route_store,
                                                                    IMessageRepository* message_repository,
                                                                    IRelationRepository* relation_repository,
                                                                    IDeliveryGateway* delivery_gateway,
                                                                    IEventPublisher* event_publisher);

std::unique_ptr<IGroupMessageService> CreateGroupMessageService(const IMessageServiceConfig& message_service_config,
                                                                IMessageRepository* message_repository,
                                                                IRelationRepository* relation_repository,
                                                                IDeliveryGateway* delivery_gateway,
                                                                IEventPublisher* event_publisher);
