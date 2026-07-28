#include "RelationServiceFactory.hpp"

#include "ChatRelationService.hpp"
#include "RelationGrpcServiceAdapter.hpp"
#include "auth/RelationGrpcAuth.hpp"
#include "logging/Logger.hpp"

import memochat.chat.service_factory_algorithms;

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
                                                        IEventPublisher* event_publisher,
                                                        std::string* error)
{
    const auto backend = relation_service_config.RelationServiceBackend();
    if (memochat::chat::factory::modules::IsInProcessBackend(backend.data(), backend.size()))
    {
        return CreateInProcessRelationService(relation_repository,
                                              relation_bootstrap_cache,
                                              delivery_gateway,
                                              task_publisher,
                                              event_publisher);
    }
    if (memochat::chat::factory::modules::IsRemoteBackend(backend.data(), backend.size()))
    {
        const auto endpoint = relation_service_config.RelationServiceEndpoint();
        const auto auth_token = relation_service_config.RelationServiceAuthToken();
        if (endpoint.empty())
        {
            const std::string message = "Relation service remote endpoint is empty: " + backend;
            if (error != nullptr)
            {
                *error = message;
            }
            memolog::LogError("chat.relation_service.endpoint_missing", message, {{"configured_backend", backend}});
            return nullptr;
        }
        if (!memochat::auth::IsStrongRelationGrpcAuthToken(auth_token))
        {
            const std::string message = "Relation service auth token must be at least 32 printable ASCII bytes";
            if (error != nullptr)
            {
                *error = message;
            }
            memolog::LogError("chat.relation_service.auth_token_invalid", message, {{"configured_backend", backend}});
            return nullptr;
        }
        return std::make_unique<RelationGrpcServiceAdapter>(endpoint, auth_token);
    }

    const std::string message = "Unsupported relation service backend: " + backend;
    if (error != nullptr)
    {
        *error = message;
    }
    memolog::LogError("chat.relation_service.unsupported_backend", message, {{"configured_backend", backend}});
    return nullptr;
}
