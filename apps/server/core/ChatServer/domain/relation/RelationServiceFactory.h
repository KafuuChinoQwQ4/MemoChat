#pragma once

#include "ports/IDeliveryGateway.h"
#include "ports/IDeliveryTaskPublisher.h"
#include "ports/IEventPublisher.h"
#include "ports/IRelationBootstrapCache.h"
#include "ports/IRelationRepository.h"
#include "ports/IRelationServiceConfig.h"
#include "ports/IRelationService.h"

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
