export module memochat.chat.relation_grpc_service_adapter_algorithms;

export namespace memochat::chat::relation_grpc_service_adapter::modules
{
constexpr unsigned RelationQueryForwardCount()
{
    return 2U;
}

constexpr unsigned RelationCommandForwardCount()
{
    return 8U;
}

constexpr unsigned TotalRelationForwardCount()
{
    return RelationQueryForwardCount() + RelationCommandForwardCount();
}

constexpr long long DefaultRelationAdapterTimeoutMs()
{
    return 2000LL;
}

constexpr bool UsesDefaultRelationAdapterTimeout(long long timeout_ms)
{
    return timeout_ms == DefaultRelationAdapterTimeoutMs();
}

constexpr bool DidForwardExpectedRelationSurface(unsigned query_count, unsigned command_count)
{
    return query_count == RelationQueryForwardCount() && command_count == RelationCommandForwardCount();
}
} // namespace memochat::chat::relation_grpc_service_adapter::modules
