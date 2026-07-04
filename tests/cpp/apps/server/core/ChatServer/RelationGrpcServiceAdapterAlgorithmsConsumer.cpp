import memochat.chat.relation_grpc_service_adapter_algorithms;

namespace modules = memochat::chat::relation_grpc_service_adapter::modules;

unsigned MemoChatTestRelationAdapterQueryForwardCount()
{
    return modules::RelationQueryForwardCount();
}

unsigned MemoChatTestRelationAdapterCommandForwardCount()
{
    return modules::RelationCommandForwardCount();
}

unsigned MemoChatTestRelationAdapterTotalForwardCount()
{
    return modules::TotalRelationForwardCount();
}

long long MemoChatTestRelationAdapterDefaultTimeoutMs()
{
    return modules::DefaultRelationAdapterTimeoutMs();
}

bool MemoChatTestRelationAdapterUsesDefaultTimeout(long long timeout_ms)
{
    return modules::UsesDefaultRelationAdapterTimeout(timeout_ms);
}

bool MemoChatTestRelationAdapterDidForwardExpectedSurface(unsigned query_count, unsigned command_count)
{
    return modules::DidForwardExpectedRelationSurface(query_count, command_count);
}
