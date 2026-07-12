export module memochat.ai.impl_algorithms;

export namespace memochat::ai::impl::modules
{
const char* DefaultInternalAuthHeader()
{
    return "X-MemoChat-AI-Internal-Key";
}

const char* DefaultInternalKeyEnv()
{
    return "MEMOCHAT_AI_INTERNAL_API_KEY";
}

bool ShouldRejectInternalAuth(bool key_configured,
                              bool supplied_empty,
                              bool token_matches,
                              bool local_environment,
                              bool loopback_bind)
{
    if (!key_configured)
    {
        return !(local_environment && loopback_bind);
    }
    return supplied_empty || !token_matches;
}

const char* RpcKind()
{
    return "gRPC";
}

const char* ChatSpan()
{
    return "AIService.Chat";
}

const char* ChatStreamSpan()
{
    return "AIService.ChatStream";
}

const char* SmartSpan()
{
    return "AIService.Smart";
}

const char* GetHistorySpan()
{
    return "AIService.GetHistory";
}

const char* CreateSessionSpan()
{
    return "AIService.CreateSession";
}

const char* ListSessionsSpan()
{
    return "AIService.ListSessions";
}

const char* DeleteSessionSpan()
{
    return "AIService.DeleteSession";
}

const char* UpdateSessionSpan()
{
    return "AIService.UpdateSession";
}

const char* ListModelsSpan()
{
    return "AIService.ListModels";
}

const char* RegisterApiProviderSpan()
{
    return "AIService.RegisterApiProvider";
}

const char* DeleteApiProviderSpan()
{
    return "AIService.DeleteApiProvider";
}

const char* KbUploadSpan()
{
    return "AIService.KbUpload";
}

const char* KbSearchSpan()
{
    return "AIService.KbSearch";
}

const char* KbListSpan()
{
    return "AIService.KbList";
}

const char* KbDeleteSpan()
{
    return "AIService.KbDelete";
}

const char* MemoryListSpan()
{
    return "AIService.MemoryList";
}

const char* MemoryCreateSpan()
{
    return "AIService.MemoryCreate";
}

const char* MemoryDeleteSpan()
{
    return "AIService.MemoryDelete";
}

const char* AgentTaskCreateSpan()
{
    return "AIService.AgentTaskCreate";
}

const char* AgentTaskListSpan()
{
    return "AIService.AgentTaskList";
}

const char* AgentTaskGetSpan()
{
    return "AIService.AgentTaskGet";
}

const char* AgentTaskCancelSpan()
{
    return "AIService.AgentTaskCancel";
}

const char* AgentTaskResumeSpan()
{
    return "AIService.AgentTaskResume";
}

const char* ConfirmSpan()
{
    return "AIService.Confirm";
}
} // namespace memochat::ai::impl::modules
