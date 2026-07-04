export module memochat.ai.gateway_client_algorithms;

// Narrow-extraction module for the AI gateway gRPC client (domain/AIServiceClient.cpp).
//
// AIServiceClient.cpp / AIServiceClient.hpp transitively include the generated
// ai_message.grpc.pb.h and <grpcpp/grpcpp.h> (heavy generated gRPC/protobuf),
// which ICE GCC 16.1.1 under -fmodules. This module therefore exports ONLY the
// pure primitive literals/guards the client uses; the real TU is NOT a module
// consumer. Production consumption is intentionally not wired (production_consumer=false).
//
// Exported surface is primitive-only: const char* literals, one int error code,
// and bool guard decisions. No JSON values, gRPC/protobuf types, ConfigMgr,
// Boost.UUID, request/response DTOs, or std::string ownership cross this boundary.

export namespace memochat::gate::services::ai::client_modules
{
// AIServer channel target defaults (host + ":" + port assembly stays in the TU).
const char* DefaultAIServerHost()
{
    return "127.0.0.1";
}

const char* DefaultAIServerPort()
{
    return "8095";
}

// The empty-string fallback guards mirror the original `if (host.empty())` /
// `if (port.empty())` decisions; the caller passes the emptiness bool.
bool ShouldUseDefaultHost(bool host_empty)
{
    return host_empty;
}

bool ShouldUseDefaultPort(bool port_empty)
{
    return port_empty;
}

// RegisterApiProvider defaults the adapter when the caller leaves it empty.
const char* DefaultApiProviderAdapter()
{
    return "openai_compatible";
}

bool ShouldUseDefaultAdapter(bool adapter_empty)
{
    return adapter_empty;
}

// Shared "AIServer gRPC status not ok" error payload.
int UnavailableErrorCode()
{
    return 500;
}

const char* UnavailableMessage()
{
    return "AIServer unavailable";
}

// Streaming: successful final chunk code, plus the fallback message prefixed to
// the gRPC status error text when the stream finishes with a non-ok status.
int StreamSuccessCode()
{
    return 0;
}

const char* StreamUnavailablePrefix()
{
    return "AIServer unavailable: ";
}

// Per-endpoint failure messages passed into the shared 500 error payload.
const char* KbUploadFailedMessage()
{
    return "upload failed";
}

const char* KbSearchFailedMessage()
{
    return "search failed";
}

const char* KbListFailedMessage()
{
    return "list failed";
}

const char* KbDeleteFailedMessage()
{
    return "delete failed";
}

const char* MemoryListFailedMessage()
{
    return "memory list failed";
}

const char* MemoryCreateFailedMessage()
{
    return "memory create failed";
}

const char* MemoryDeleteFailedMessage()
{
    return "memory delete failed";
}

const char* AgentTaskCreateFailedMessage()
{
    return "task create failed";
}

const char* AgentTaskListFailedMessage()
{
    return "task list failed";
}

const char* AgentTaskGetFailedMessage()
{
    return "task get failed";
}

const char* AgentTaskCancelFailedMessage()
{
    return "task cancel failed";
}

const char* AgentTaskResumeFailedMessage()
{
    return "task resume failed";
}
} // namespace memochat::gate::services::ai::client_modules
