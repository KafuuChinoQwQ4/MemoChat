#include "RelationQueryServiceFactory.h"

#include "RelationQueryGrpcClient.h"
#include "logging/Logger.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace
{
bool HasRelationQueryRemoteError(const memochat::json::JsonValue& out)
{
    return out.isMember("relation_query_remote_error") || out.isMember("relation_query_remote_status_code") ||
           out.isMember("relation_query_remote_error_code");
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
        if (!HasRelationQueryRemoteError(out) || _fallback == nullptr)
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
        auto primary = CreateRemoteRelationQueryService(relation_query_service_config);
        if (inprocess_relation_query_service != nullptr)
        {
            remote_relation_query_service =
                std::make_unique<FallbackRelationQueryService>(std::move(primary),
                                                               inprocess_relation_query_service,
                                                               backend);
        }
        else
        {
            remote_relation_query_service = std::move(primary);
        }
        return remote_relation_query_service.get();
    }

    memolog::LogWarn("chat.relation_query_service.unsupported_backend",
                     "relation query service backend is not implemented yet, fallback to inprocess",
                     {{"configured_backend", backend}, {"fallback_backend", "inprocess"}});
    return inprocess_relation_query_service;
}
