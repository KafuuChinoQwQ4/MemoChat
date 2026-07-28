#include "RelationQueryServiceFactory.hpp"

#include "RelationQueryGrpcClient.hpp"
#include "auth/RelationGrpcAuth.hpp"
#include "logging/Logger.hpp"

#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

import memochat.chat.service_factory_algorithms;

namespace
{
bool EnvironmentFlagEnabled(const char* name)
{
    const char* value = std::getenv(name);
    return value != nullptr && std::string_view(value) == "1";
}

bool InProcessFallbackExplicitlyEnabled()
{
    return EnvironmentFlagEnabled("MEMOCHAT_RELATION_QUERY_ALLOW_INPROCESS_FALLBACK") &&
           !EnvironmentFlagEnabled("MEMOCHAT_RELEASE_MODE");
}

bool HasRetryableRelationQueryRemoteError(const memochat::json::JsonValue& out)
{
    if (!out.isMember("relation_query_remote_status_code"))
    {
        return false;
    }
    const auto status = static_cast<grpc::StatusCode>(out["relation_query_remote_status_code"].asInt());
    return status == grpc::StatusCode::UNAVAILABLE || status == grpc::StatusCode::DEADLINE_EXCEEDED;
}

class FallbackRelationQueryService final : public IRelationQueryService
{
public:
    FallbackRelationQueryService(std::unique_ptr<IRelationQueryService> primary,
                                 IRelationQueryService* fallback,
                                 std::string backend)
        : _primary(std::move(primary))
        , _fallback(fallback)
        , _backend(std::move(backend))
    {
    }

    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override
    {
        CallWithFallback(
            uid,
            out,
            "AppendRelationBootstrap",
            [this](int fallback_uid, memochat::json::JsonValue& fallback_out)
            {
                _fallback->AppendRelationBootstrapJson(fallback_uid, fallback_out);
            },
            [this](int primary_uid, memochat::json::JsonValue& primary_out)
            {
                _primary->AppendRelationBootstrapJson(primary_uid, primary_out);
            });
    }

    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override
    {
        CallWithFallback(
            uid,
            out,
            "BuildDialogList",
            [this](int fallback_uid, memochat::json::JsonValue& fallback_out)
            {
                _fallback->BuildDialogListJson(fallback_uid, fallback_out);
            },
            [this](int primary_uid, memochat::json::JsonValue& primary_out)
            {
                _primary->BuildDialogListJson(primary_uid, primary_out);
            });
    }

private:
    template <typename FallbackFn, typename PrimaryFn>
    void CallWithFallback(int uid,
                          memochat::json::JsonValue& out,
                          const char* method_name,
                          FallbackFn fallback_fn,
                          PrimaryFn primary_fn)
    {
        const memochat::json::JsonValue original_out = out;
        primary_fn(uid, out);
        if (!HasRetryableRelationQueryRemoteError(out) || _fallback == nullptr)
        {
            return;
        }

        memolog::LogWarn("chat.relation_query_service.remote_fallback",
                         "relation query remote failed, fallback to inprocess",
                         {{"configured_backend", _backend},
                          {"method", method_name},
                          {"uid", std::to_string(uid)},
                          {"remote_error", out["relation_query_remote_error"].asString()}});

        memochat::json::JsonValue fallback_out = original_out;
        fallback_fn(uid, fallback_out);
        out = fallback_out;
    }

    std::unique_ptr<IRelationQueryService> _primary;
    IRelationQueryService* _fallback = nullptr;
    std::string _backend;
};
} // namespace

std::unique_ptr<IRelationQueryService>
CreateRemoteRelationQueryService(const IRelationQueryServiceConfig& relation_query_service_config, std::string* error)
{
    const auto backend = relation_query_service_config.RelationQueryServiceBackend();
    const auto endpoint = relation_query_service_config.RelationQueryServiceEndpoint();
    const auto auth_token = relation_query_service_config.RelationQueryServiceChatAuthToken();
    if (endpoint.empty())
    {
        const std::string message = "Relation query service remote endpoint is empty: " + backend;
        if (error != nullptr)
        {
            *error = message;
        }
        memolog::LogError("chat.relation_query_service.endpoint_missing", message, {{"configured_backend", backend}});
        return nullptr;
    }
    if (!memochat::auth::IsStrongRelationGrpcAuthToken(auth_token))
    {
        const std::string message = "Relation query service chat auth token must be at least 32 printable ASCII bytes";
        if (error != nullptr)
        {
            *error = message;
        }
        memolog::LogError("chat.relation_query_service.auth_token_invalid", message, {{"configured_backend", backend}});
        return nullptr;
    }
    return std::make_unique<RelationQueryGrpcClient>(endpoint, auth_token);
}

IRelationQueryService* SelectRelationQueryService(const IRelationQueryServiceConfig& relation_query_service_config,
                                                  IRelationQueryService* inprocess_relation_query_service,
                                                  std::unique_ptr<IRelationQueryService>& remote_relation_query_service,
                                                  std::string* error)
{
    remote_relation_query_service.reset();

    const auto backend = relation_query_service_config.RelationQueryServiceBackend();
    if (memochat::chat::factory::modules::IsInProcessBackend(backend.data(), backend.size()))
    {
        return inprocess_relation_query_service;
    }
    if (memochat::chat::factory::modules::IsRemoteBackend(backend.data(), backend.size()))
    {
        auto primary = CreateRemoteRelationQueryService(relation_query_service_config, error);
        if (!primary)
        {
            return nullptr;
        }
        if (inprocess_relation_query_service != nullptr && InProcessFallbackExplicitlyEnabled())
        {
            remote_relation_query_service =
                std::make_unique<FallbackRelationQueryService>(std::move(primary),
                                                               inprocess_relation_query_service,
                                                               backend);
        }
        else
        {
            if (inprocess_relation_query_service != nullptr)
            {
                memolog::LogInfo("chat.relation_query_service.fallback_disabled",
                                 "in-process relation query fallback is disabled",
                                 {{"configured_backend", backend}});
            }
            remote_relation_query_service = std::move(primary);
        }
        return remote_relation_query_service.get();
    }

    const std::string message = "Unsupported relation query service backend: " + backend;
    if (error != nullptr)
    {
        *error = message;
    }
    memolog::LogError("chat.relation_query_service.unsupported_backend", message, {{"configured_backend", backend}});
    return nullptr;
}
