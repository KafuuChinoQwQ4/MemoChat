export module memochat.chat.service_factory_algorithms;

export namespace memochat::chat::factory::modules
{
bool EqualsAscii(const char* data, unsigned long long size, const char* literal, unsigned long long literal_size)
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

bool IsInProcessBackend(const char* data, unsigned long long size)
{
    return EqualsAscii(data, size, "inprocess", 9);
}

bool IsRemoteBackend(const char* data, unsigned long long size)
{
    return EqualsAscii(data, size, "grpc", 4) || EqualsAscii(data, size, "remote", 6);
}
} // namespace memochat::chat::factory::modules
