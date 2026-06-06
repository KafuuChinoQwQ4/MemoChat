#pragma once

#include "ports/IDeliveryGateway.h"
#include "ports/IEventPublisher.h"
#include "ports/IGroupMessageService.h"
#include "ports/IMessageRepository.h"
#include "ports/IMessageServiceConfig.h"
#include "ports/IOnlineRouteStore.h"
#include "ports/IPrivateMessageService.h"
#include "ports/IRelationRepository.h"
#include "ports/ISessionRegistry.h"

#include <memory>

std::unique_ptr<IPrivateMessageService> CreateInProcessPrivateMessageService(ISessionRegistry* session_registry,
                                                                             IOnlineRouteStore* online_route_store,
                                                                             IMessageRepository* message_repository,
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
                                                                    IDeliveryGateway* delivery_gateway,
                                                                    IEventPublisher* event_publisher);

std::unique_ptr<IGroupMessageService> CreateGroupMessageService(const IMessageServiceConfig& message_service_config,
                                                                IMessageRepository* message_repository,
                                                                IRelationRepository* relation_repository,
                                                                IDeliveryGateway* delivery_gateway,
                                                                IEventPublisher* event_publisher);
