export module memochat.ai.conversation_context_algorithms;

export namespace memochat::ai::conversation_context::modules
{
const char* UserRole()
{
    return "user";
}

const char* AssistantRole()
{
    return "assistant";
}

bool ShouldPruneMessages(unsigned long long message_count, unsigned long long max_size)
{
    return message_count > max_size;
}
} // namespace memochat::ai::conversation_context::modules
