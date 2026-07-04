export module memochat.chat.chat_grpc_client_algorithms;

export namespace memochat::chat::chat_grpc_client::modules
{
bool EqualsLiteral(const char* data, unsigned long long size, const char* literal, unsigned long long literal_size)
{
    if (data == nullptr || literal == nullptr || size != literal_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < size; ++index)
    {
        if (data[index] != literal[index])
        {
            return false;
        }
    }
    return true;
}

bool ShouldSkipSelfNode(const char* node_name,
                        unsigned long long node_name_size,
                        const char* self_name,
                        unsigned long long self_name_size)
{
    return EqualsLiteral(node_name, node_name_size, self_name, self_name_size);
}

int DefaultRemotePoolSize()
{
    return 5;
}

bool ShouldSkipRemoteCall(bool pool_found)
{
    return !pool_found;
}

bool ShouldReportGrpcStatusError(bool status_ok)
{
    return !status_ok;
}
} // namespace memochat::chat::chat_grpc_client::modules
