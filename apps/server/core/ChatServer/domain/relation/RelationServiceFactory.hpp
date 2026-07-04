#pragma once

#include "ports/IDeliveryGateway.hpp"
#include "ports/IDeliveryTaskPublisher.hpp"
#include "ports/IEventPublisher.hpp"
#include "ports/IRelationBootstrapCache.hpp"
#include "ports/IRelationRepository.hpp"
#include "ports/IRelationServiceConfig.hpp"
#include "ports/IRelationService.hpp"

#include <memory>

std::unique_ptr<IRelationService> CreateInProcessRelationService(IRelationRepository* relation_repository,
                                                                 IRelationBootstrapCache* relation_bootstrap_cache,
                                                                 IDeliveryGateway* delivery_gateway,
                                                                 IDeliveryTaskPublisher* task_publisher,
                                                                 IEventPublisher* event_publisher);

std::unique_ptr<IRelationService> CreateRelationService(const IRelationServiceConfig& relation_service_config,
                                                        IRelationRepository* relation_repository,
                                                        IRelationBootstrapCache* relation_bootstrap_cache,
                                                        IDeliveryGateway* delivery_gateway,
                                                        IDeliveryTaskPublisher* task_publisher,
                                                        IEventPublisher* event_publisher);
