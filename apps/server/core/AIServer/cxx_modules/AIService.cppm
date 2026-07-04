export module memochat.ai.service_algorithms;

export namespace memochat::ai::service::modules
{
bool IsSuccessfulModelListPayload(bool payload_is_object, int code)
{
    return payload_is_object && code == 0;
}

int NormalizeAgentTaskListLimit(int limit, int default_limit)
{
    if (limit <= 0)
    {
        return default_limit;
    }
    return limit;
}
} // namespace memochat::ai::service::modules
