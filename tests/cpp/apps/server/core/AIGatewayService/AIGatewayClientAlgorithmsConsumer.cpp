import memochat.ai.gateway_client_algorithms;

namespace memochat::tests::ai::gateway_client
{
const char* DefaultAIServerHost()
{
    return memochat::gate::services::ai::client_modules::DefaultAIServerHost();
}

const char* DefaultAIServerPort()
{
    return memochat::gate::services::ai::client_modules::DefaultAIServerPort();
}

const char* DefaultAIServerInternalAuthHeader()
{
    return memochat::gate::services::ai::client_modules::DefaultAIServerInternalAuthHeader();
}

const char* DefaultAIServerInternalKeyEnv()
{
    return memochat::gate::services::ai::client_modules::DefaultAIServerInternalKeyEnv();
}

bool ShouldRejectAIServerAuthConfiguration(bool key_configured, bool local_environment, bool loopback_target)
{
    return memochat::gate::services::ai::client_modules::ShouldRejectAIServerAuthConfiguration(key_configured,
                                                                                               local_environment,
                                                                                               loopback_target);
}

bool ShouldUseDefaultHost(bool host_empty)
{
    return memochat::gate::services::ai::client_modules::ShouldUseDefaultHost(host_empty);
}

bool ShouldUseDefaultPort(bool port_empty)
{
    return memochat::gate::services::ai::client_modules::ShouldUseDefaultPort(port_empty);
}

const char* DefaultApiProviderAdapter()
{
    return memochat::gate::services::ai::client_modules::DefaultApiProviderAdapter();
}

bool ShouldUseDefaultAdapter(bool adapter_empty)
{
    return memochat::gate::services::ai::client_modules::ShouldUseDefaultAdapter(adapter_empty);
}

int UnavailableErrorCode()
{
    return memochat::gate::services::ai::client_modules::UnavailableErrorCode();
}

const char* UnavailableMessage()
{
    return memochat::gate::services::ai::client_modules::UnavailableMessage();
}

int StreamSuccessCode()
{
    return memochat::gate::services::ai::client_modules::StreamSuccessCode();
}

const char* StreamUnavailablePrefix()
{
    return memochat::gate::services::ai::client_modules::StreamUnavailablePrefix();
}

const char* KbUploadFailedMessage()
{
    return memochat::gate::services::ai::client_modules::KbUploadFailedMessage();
}

const char* KbSearchFailedMessage()
{
    return memochat::gate::services::ai::client_modules::KbSearchFailedMessage();
}

const char* KbListFailedMessage()
{
    return memochat::gate::services::ai::client_modules::KbListFailedMessage();
}

const char* KbDeleteFailedMessage()
{
    return memochat::gate::services::ai::client_modules::KbDeleteFailedMessage();
}

const char* MemoryListFailedMessage()
{
    return memochat::gate::services::ai::client_modules::MemoryListFailedMessage();
}

const char* MemoryCreateFailedMessage()
{
    return memochat::gate::services::ai::client_modules::MemoryCreateFailedMessage();
}

const char* MemoryDeleteFailedMessage()
{
    return memochat::gate::services::ai::client_modules::MemoryDeleteFailedMessage();
}

const char* AgentTaskCreateFailedMessage()
{
    return memochat::gate::services::ai::client_modules::AgentTaskCreateFailedMessage();
}

const char* AgentTaskListFailedMessage()
{
    return memochat::gate::services::ai::client_modules::AgentTaskListFailedMessage();
}

const char* AgentTaskGetFailedMessage()
{
    return memochat::gate::services::ai::client_modules::AgentTaskGetFailedMessage();
}

const char* AgentTaskCancelFailedMessage()
{
    return memochat::gate::services::ai::client_modules::AgentTaskCancelFailedMessage();
}

const char* AgentTaskResumeFailedMessage()
{
    return memochat::gate::services::ai::client_modules::AgentTaskResumeFailedMessage();
}
} // namespace memochat::tests::ai::gateway_client
