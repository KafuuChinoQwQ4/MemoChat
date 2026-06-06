#include "RelationQueryServiceFactory.h"

#include "RelationQueryGrpcClient.h"
#include "logging/Logger.h"

#include <stdexcept>

std::unique_ptr<IRelationQueryService>
CreateRemoteRelationQueryService(const IRelationQueryServiceConfig& relation_query_service_config)
{
    const auto backend = relation_query_service_config.RelationQueryServiceBackend();
    const auto endpoint = relation_query_service_config.RelationQueryServiceEndpoint();
    if (endpoint.empty())
    {
        throw std::runtime_error("Relation query service remote endpoint is empty: " + backend);
    }
    return std::make_unique<RelationQueryGrpcClient>(endpoint);
}

IRelationQueryService* SelectRelationQueryService(const IRelationQueryServiceConfig& relation_query_service_config,
                                                  IRelationQueryService* inprocess_relation_query_service,
                                                  std::unique_ptr<IRelationQueryService>& remote_relation_query_service)
{
    remote_relation_query_service.reset();

    const auto backend = relation_query_service_config.RelationQueryServiceBackend();
    if (backend == "inprocess")
    {
        return inprocess_relation_query_service;
    }
    if (backend == "grpc" || backend == "remote")
    {
        remote_relation_query_service = CreateRemoteRelationQueryService(relation_query_service_config);
        return remote_relation_query_service.get();
    }

    memolog::LogWarn("chat.relation_query_service.unsupported_backend",
                     "relation query service backend is not implemented yet, fallback to inprocess",
                     {{"configured_backend", backend}, {"fallback_backend", "inprocess"}});
    return inprocess_relation_query_service;
}
