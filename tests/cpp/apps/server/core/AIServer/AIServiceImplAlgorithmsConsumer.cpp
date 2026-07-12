import memochat.ai.impl_algorithms;

namespace memochat::tests::ai::impl
{
const char* RpcKind()
{
    return memochat::ai::impl::modules::RpcKind();
}

const char* DefaultInternalAuthHeader()
{
    return memochat::ai::impl::modules::DefaultInternalAuthHeader();
}

const char* DefaultInternalKeyEnv()
{
    return memochat::ai::impl::modules::DefaultInternalKeyEnv();
}

bool ShouldRejectInternalAuth(bool key_configured,
                              bool supplied_empty,
                              bool token_matches,
                              bool local_environment,
                              bool loopback_bind)
{
    return memochat::ai::impl::modules::ShouldRejectInternalAuth(key_configured,
                                                                 supplied_empty,
                                                                 token_matches,
                                                                 local_environment,
                                                                 loopback_bind);
}

const char* ChatSpan()
{
    return memochat::ai::impl::modules::ChatSpan();
}

const char* ChatStreamSpan()
{
    return memochat::ai::impl::modules::ChatStreamSpan();
}

const char* SmartSpan()
{
    return memochat::ai::impl::modules::SmartSpan();
}

const char* GetHistorySpan()
{
    return memochat::ai::impl::modules::GetHistorySpan();
}

const char* CreateSessionSpan()
{
    return memochat::ai::impl::modules::CreateSessionSpan();
}

const char* ListSessionsSpan()
{
    return memochat::ai::impl::modules::ListSessionsSpan();
}

const char* DeleteSessionSpan()
{
    return memochat::ai::impl::modules::DeleteSessionSpan();
}

const char* UpdateSessionSpan()
{
    return memochat::ai::impl::modules::UpdateSessionSpan();
}

const char* ListModelsSpan()
{
    return memochat::ai::impl::modules::ListModelsSpan();
}

const char* RegisterApiProviderSpan()
{
    return memochat::ai::impl::modules::RegisterApiProviderSpan();
}

const char* DeleteApiProviderSpan()
{
    return memochat::ai::impl::modules::DeleteApiProviderSpan();
}

const char* KbUploadSpan()
{
    return memochat::ai::impl::modules::KbUploadSpan();
}

const char* KbSearchSpan()
{
    return memochat::ai::impl::modules::KbSearchSpan();
}

const char* KbListSpan()
{
    return memochat::ai::impl::modules::KbListSpan();
}

const char* KbDeleteSpan()
{
    return memochat::ai::impl::modules::KbDeleteSpan();
}

const char* MemoryListSpan()
{
    return memochat::ai::impl::modules::MemoryListSpan();
}

const char* MemoryCreateSpan()
{
    return memochat::ai::impl::modules::MemoryCreateSpan();
}

const char* MemoryDeleteSpan()
{
    return memochat::ai::impl::modules::MemoryDeleteSpan();
}

const char* AgentTaskCreateSpan()
{
    return memochat::ai::impl::modules::AgentTaskCreateSpan();
}

const char* AgentTaskListSpan()
{
    return memochat::ai::impl::modules::AgentTaskListSpan();
}

const char* AgentTaskGetSpan()
{
    return memochat::ai::impl::modules::AgentTaskGetSpan();
}

const char* AgentTaskCancelSpan()
{
    return memochat::ai::impl::modules::AgentTaskCancelSpan();
}

const char* AgentTaskResumeSpan()
{
    return memochat::ai::impl::modules::AgentTaskResumeSpan();
}

const char* ConfirmSpan()
{
    return memochat::ai::impl::modules::ConfirmSpan();
}
} // namespace memochat::tests::ai::impl
