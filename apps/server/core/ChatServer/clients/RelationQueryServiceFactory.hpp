#pragma once

#include "ports/IRelationQueryService.hpp"
#include "ports/IRelationQueryServiceConfig.hpp"

#include <memory>
#include <string>

std::unique_ptr<IRelationQueryService>
CreateRemoteRelationQueryService(const IRelationQueryServiceConfig& relation_query_service_config,
                                 std::string* error = nullptr);

IRelationQueryService* SelectRelationQueryService(const IRelationQueryServiceConfig& relation_query_service_config,
                                                  IRelationQueryService* inprocess_relation_query_service,
                                                  std::unique_ptr<IRelationQueryService>& remote_relation_query_service,
                                                  std::string* error = nullptr);
