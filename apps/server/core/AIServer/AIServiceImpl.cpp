#include "AIServiceImpl.hpp"
#include "AIServiceCore.hpp"
#include "ConfigMgr.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

import memochat.ai.impl_algorithms;

namespace impl_modules = memochat::ai::impl::modules;

namespace
{
std::string TrimAscii(std::string value)
{
    auto is_space = [](unsigned char c)
    {
        return std::isspace(c) != 0;
    };
    value.erase(value.begin(),
                std::find_if(value.begin(),
                             value.end(),
                             [&](char c)
                             {
                                 return !is_space(static_cast<unsigned char>(c));
                             }));
    value.erase(std::find_if(value.rbegin(),
                             value.rend(),
                             [&](char c)
                             {
                                 return !is_space(static_cast<unsigned char>(c));
                             })
                    .base(),
                value.end());
    return value;
}

std::string LowerAscii(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    return value;
}

std::string ConfigValue(std::string_view section, std::string_view key)
{
    return TrimAscii(ConfigMgr::Inst().GetValue(std::string(section), std::string(key)));
}

std::string EnvValue(const std::string& name)
{
    if (name.empty())
    {
        return "";
    }
    const char* value = std::getenv(name.c_str());
    return value == nullptr ? "" : TrimAscii(value);
}

bool IsLocalEnvironment()
{
    std::string environment = EnvValue("MEMOCHAT_ENV");
    if (environment.empty())
    {
        environment = ConfigValue("Log", "Env");
    }
    environment = LowerAscii(std::move(environment));
    return environment == "local" || environment == "dev" || environment == "development" || environment == "test";
}

bool IsLoopbackBind()
{
    std::string host = LowerAscii(ConfigValue("AIServer", "Host"));
    return host == "127.0.0.1" || host == "localhost" || host == "::1" || host == "[::1]";
}

std::string AIServerInternalMetadataKey()
{
    std::string header = ConfigValue("AIServer", "InternalAuthHeader");
    if (header.empty())
    {
        header = impl_modules::DefaultInternalAuthHeader();
    }
    return LowerAscii(header);
}

std::string ResolveAIServerInternalKey()
{
    std::string env_name = ConfigValue("AIServer", "InternalApiKeyEnv");
    if (env_name.empty())
    {
        env_name = impl_modules::DefaultInternalKeyEnv();
    }
    if (const std::string env_value = EnvValue(env_name); !env_value.empty())
    {
        return env_value;
    }
    return ConfigValue("AIServer", "InternalApiKey");
}

std::string ProviderAdminMetadataKey()
{
    std::string header = ConfigValue("AIProviderAdmin", "AuthHeader");
    if (header.empty())
    {
        header = "X-MemoChat-AI-Provider-Admin-Key";
    }
    return LowerAscii(header);
}

std::string ResolveProviderAdminKey()
{
    std::string env_name = ConfigValue("AIProviderAdmin", "AdminKeyEnv");
    if (env_name.empty())
    {
        env_name = "MEMOCHAT_AI_PROVIDER_ADMIN_KEY";
    }
    if (const std::string env_value = EnvValue(env_name); !env_value.empty())
    {
        return env_value;
    }
    return ConfigValue("AIProviderAdmin", "AdminKey");
}

bool ConstantTimeEquals(std::string_view left, std::string_view right)
{
    unsigned char diff = static_cast<unsigned char>(left.size() ^ right.size());
    const std::size_t max_size = std::max(left.size(), right.size());
    for (std::size_t index = 0; index < max_size; ++index)
    {
        const unsigned char l = index < left.size() ? static_cast<unsigned char>(left[index]) : 0;
        const unsigned char r = index < right.size() ? static_cast<unsigned char>(right[index]) : 0;
        diff |= static_cast<unsigned char>(l ^ r);
    }
    return diff == 0;
}

std::string MetadataValue(const ServerContext* context, const std::string& key)
{
    if (context == nullptr)
    {
        return "";
    }
    const auto& metadata = context->client_metadata();
    const auto range = metadata.equal_range(grpc::string_ref(key.data(), key.size()));
    if (range.first == range.second)
    {
        return "";
    }
    return TrimAscii(std::string(range.first->second.data(), range.first->second.length()));
}

grpc::Status RequireAIServerInternalMetadata(const ServerContext* context)
{
    const std::string configured_key = ResolveAIServerInternalKey();
    const std::string supplied = MetadataValue(context, AIServerInternalMetadataKey());
    const bool token_matches =
        !configured_key.empty() && !supplied.empty() && ConstantTimeEquals(supplied, configured_key);
    if (!impl_modules::ShouldRejectInternalAuth(!configured_key.empty(),
                                                supplied.empty(),
                                                token_matches,
                                                IsLocalEnvironment(),
                                                IsLoopbackBind()))
    {
        return grpc::Status::OK;
    }

    if (configured_key.empty())
    {
        memolog::LogError("ai.grpc.internal_auth_misconfigured",
                          "AIServer internal API key is required outside local loopback development",
                          {{"bind_host", ConfigValue("AIServer", "Host")}});
    }
    else
    {
        memolog::LogWarn("ai.grpc.internal_auth_rejected", "rejected unauthenticated AIServer gRPC caller", {});
    }
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "AIServer internal auth required");
}

grpc::Status RequireProviderAdminMetadata(const ServerContext* context)
{
    const std::string configured_key = ResolveProviderAdminKey();
    const std::string supplied = MetadataValue(context, ProviderAdminMetadataKey());
    if (configured_key.empty() || supplied.empty() || !ConstantTimeEquals(supplied, configured_key))
    {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "provider admin auth required");
    }
    return grpc::Status::OK;
}
} // namespace

AIServiceImpl::AIServiceImpl()
    : _core(std::make_unique<AIServiceCore>())
{
}

AIServiceImpl::~AIServiceImpl() = default;

grpc::Status AIServiceImpl::Chat(ServerContext* context, const ai::AIChatReq* request, ai::AIChatRsp* reply)
{
    memolog::SpanScope span(impl_modules::ChatSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->HandleChat(*request, reply);
}

grpc::Status AIServiceImpl::ChatStream(ServerContext* context,
                                       const ai::AIChatStreamReq* request,
                                       grpc::ServerWriter<ai::AIChatStreamChunk>* writer)
{
    memolog::SpanScope span(impl_modules::ChatStreamSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->HandleChatStream(request->req(), writer);
}

grpc::Status AIServiceImpl::Smart(ServerContext* context, const ai::AISmartReq* request, ai::AISmartRsp* reply)
{
    memolog::SpanScope span(impl_modules::SmartSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->HandleSmart(*request, reply);
}

grpc::Status AIServiceImpl::GetHistory(ServerContext* context, const ai::AIHistoryReq* request, ai::AIHistoryRsp* reply)
{
    memolog::SpanScope span(impl_modules::GetHistorySpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->GetHistory(*request, reply);
}

grpc::Status
AIServiceImpl::CreateSession(ServerContext* context, const ai::AICreateSessionReq* request, ai::AISessionRsp* reply)
{
    memolog::SpanScope span(impl_modules::CreateSessionSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->CreateSession(*request, reply);
}

grpc::Status
AIServiceImpl::ListSessions(ServerContext* context, const ai::AICreateSessionReq* request, ai::AISessionRsp* reply)
{
    memolog::SpanScope span(impl_modules::ListSessionsSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->ListSessions(*request, reply);
}

grpc::Status AIServiceImpl::DeleteSession(ServerContext* context,
                                          const ai::AIDeleteSessionReq* request,
                                          ai::AIDeleteSessionRsp* reply)
{
    memolog::SpanScope span(impl_modules::DeleteSessionSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->DeleteSession(*request, reply);
}

grpc::Status
AIServiceImpl::UpdateSession(ServerContext* context, const ai::AIUpdateSessionReq* request, ai::AISessionRsp* reply)
{
    memolog::SpanScope span(impl_modules::UpdateSessionSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->UpdateSession(*request, reply);
}

grpc::Status
AIServiceImpl::ListModels(ServerContext* context, const ai::AIListModelsReq* request, ai::AIListModelsRsp* reply)
{
    memolog::SpanScope span(impl_modules::ListModelsSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->ListModels(*request, reply);
}

grpc::Status AIServiceImpl::RegisterApiProvider(ServerContext* context,
                                                const ai::AIRegisterApiProviderReq* request,
                                                ai::AIRegisterApiProviderRsp* reply)
{
    memolog::SpanScope span(impl_modules::RegisterApiProviderSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    if (const auto auth_status = RequireProviderAdminMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->RegisterApiProvider(*request, reply);
}

grpc::Status AIServiceImpl::DeleteApiProvider(ServerContext* context,
                                              const ai::AIDeleteApiProviderReq* request,
                                              ai::AIDeleteApiProviderRsp* reply)
{
    memolog::SpanScope span(impl_modules::DeleteApiProviderSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    if (const auto auth_status = RequireProviderAdminMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->DeleteApiProvider(*request, reply);
}

grpc::Status AIServiceImpl::KbUpload(ServerContext* context, const ai::AIKbUploadReq* request, ai::AIKbUploadRsp* reply)
{
    memolog::SpanScope span(impl_modules::KbUploadSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->HandleKbUpload(*request, reply);
}

grpc::Status AIServiceImpl::KbSearch(ServerContext* context, const ai::AIKbSearchReq* request, ai::AIKbSearchRsp* reply)
{
    memolog::SpanScope span(impl_modules::KbSearchSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->HandleKbSearch(*request, reply);
}

grpc::Status AIServiceImpl::KbList(ServerContext* context, const ai::AIKbListReq* request, ai::AIKbListRsp* reply)
{
    memolog::SpanScope span(impl_modules::KbListSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->ListKb(*request, reply);
}

grpc::Status AIServiceImpl::KbDelete(ServerContext* context, const ai::AIKbDeleteReq* request, ai::AIKbDeleteRsp* reply)
{
    memolog::SpanScope span(impl_modules::KbDeleteSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->DeleteKb(*request, reply);
}

grpc::Status
AIServiceImpl::MemoryList(ServerContext* context, const ai::AIMemoryReq* request, ai::AIMemoryListRsp* reply)
{
    memolog::SpanScope span(impl_modules::MemoryListSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->MemoryList(*request, reply);
}

grpc::Status AIServiceImpl::MemoryCreate(ServerContext* context, const ai::AIMemoryReq* request, ai::AIMemoryRsp* reply)
{
    memolog::SpanScope span(impl_modules::MemoryCreateSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->MemoryCreate(*request, reply);
}

grpc::Status AIServiceImpl::MemoryDelete(ServerContext* context, const ai::AIMemoryReq* request, ai::AIMemoryRsp* reply)
{
    memolog::SpanScope span(impl_modules::MemoryDeleteSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->MemoryDelete(*request, reply);
}

grpc::Status
AIServiceImpl::AgentTaskCreate(ServerContext* context, const ai::AIAgentTaskReq* request, ai::AIAgentTaskRsp* reply)
{
    memolog::SpanScope span(impl_modules::AgentTaskCreateSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->AgentTaskCreate(*request, reply);
}

grpc::Status
AIServiceImpl::AgentTaskList(ServerContext* context, const ai::AIAgentTaskReq* request, ai::AIAgentTaskRsp* reply)
{
    memolog::SpanScope span(impl_modules::AgentTaskListSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->AgentTaskList(*request, reply);
}

grpc::Status
AIServiceImpl::AgentTaskGet(ServerContext* context, const ai::AIAgentTaskReq* request, ai::AIAgentTaskRsp* reply)
{
    memolog::SpanScope span(impl_modules::AgentTaskGetSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->AgentTaskGet(*request, reply);
}

grpc::Status
AIServiceImpl::AgentTaskCancel(ServerContext* context, const ai::AIAgentTaskReq* request, ai::AIAgentTaskRsp* reply)
{
    memolog::SpanScope span(impl_modules::AgentTaskCancelSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->AgentTaskCancel(*request, reply);
}

grpc::Status
AIServiceImpl::AgentTaskResume(ServerContext* context, const ai::AIAgentTaskReq* request, ai::AIAgentTaskRsp* reply)
{
    memolog::SpanScope span(impl_modules::AgentTaskResumeSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->AgentTaskResume(*request, reply);
}

grpc::Status AIServiceImpl::Confirm(ServerContext* context, const ai::AIConfirmReq* request, ai::AIConfirmRsp* reply)
{
    memolog::SpanScope span(impl_modules::ConfirmSpan(), impl_modules::RpcKind());
    if (const auto auth_status = RequireAIServerInternalMetadata(context); !auth_status.ok())
    {
        return auth_status;
    }
    return _core->HandleConfirm(*request, reply);
}
