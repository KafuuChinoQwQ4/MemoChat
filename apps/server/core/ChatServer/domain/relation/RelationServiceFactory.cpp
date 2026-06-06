#include "RelationServiceFactory.h"

#include "ChatRelationService.h"
#include "RelationGrpcServiceAdapter.h"
#include "logging/Logger.h"

#include <stdexcept>

std::unique_ptr<IRelationService> CreateInProcessRelationService(IRelationRepository* relation_repository,
                                                                 IRelationBootstrapCache* relation_bootstrap_cache,
                                                                 IDeliveryGateway* delivery_gateway,
                                                                 IDeliveryTaskPublisher* task_publisher,
                                                                 IEventPublisher* event_publisher)
{
    return std::make_unique<ChatRelationService>(relation_repository,
                                                 relation_bootstrap_cache,
                                                 delivery_gateway,
                                                 task_publisher,
                                                 event_publisher);
}

std::unique_ptr<IRelationService> CreateRelationService(const IRelationServiceConfig& relation_service_config,
                                                        IRelationRepository* relation_repository,
                                                        IRelationBootstrapCache* relation_bootstrap_cache,
                                                        IDeliveryGateway* delivery_gateway,
                                                        IDeliveryTaskPublisher* task_publisher,
                                                        IEventPublisher* event_publisher)
{
    const auto backend = relation_service_config.RelationServiceBackend();
    if (backend == "inprocess")
    {
        return CreateInProcessRelationService(relation_repository,
                                              relation_bootstrap_cache,
                                              delivery_gateway,
                                              task_publisher,
                                              event_publisher);
    }
    if (backend == "grpc" || backend == "remote")
    {
        const auto endpoint = relation_service_config.RelationServiceEndpoint();
        if (endpoint.empty())
        {
            throw std::runtime_error("Relation service remote endpoint is empty: " + backend);
        }
        return std::make_unique<RelationGrpcServiceAdapter>(endpoint);
    }

    memolog::LogWarn("chat.relation_service.unsupported_backend",
                     "relation service backend is not implemented yet, fallback to inprocess",
                     {{"configured_backend", backend}, {"fallback_backend", "inprocess"}});
    return CreateInProcessRelationService(relation_repository,
                                          relation_bootstrap_cache,
                                          delivery_gateway,
                                          task_publisher,
                                          event_publisher);
}
