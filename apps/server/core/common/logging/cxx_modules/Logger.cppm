export module memochat.logging.logger_algorithms;

export namespace memochat::logging::logger_modules
{
bool ShouldUseDefaultLogDir(bool dir_empty)
{
    return dir_empty;
}

bool IsTopLevelFieldName(const char* key, unsigned long long size)
{
    if (key == nullptr)
    {
        return false;
    }

    auto equals_literal = [key, size](const char* literal, unsigned long long literal_size)
    {
        if (size != literal_size)
        {
            return false;
        }
        for (unsigned long long index = 0; index < size; ++index)
        {
            if (key[index] != literal[index])
            {
                return false;
            }
        }
        return true;
    };

    return equals_literal("service_instance", 16) || equals_literal("module", 6) ||
           equals_literal("peer_service", 12) || equals_literal("error_code", 10) ||
           equals_literal("error_type", 10) || equals_literal("duration_ms", 11) ||
           equals_literal("uid", 3) || equals_literal("session_id", 10);
}
} // namespace memochat::logging::logger_modules
