import memochat.logging.logger_algorithms;

namespace memochat::tests::logging
{
bool ShouldUseDefaultLogDir(bool dir_empty)
{
    return memochat::logging::logger_modules::ShouldUseDefaultLogDir(dir_empty);
}

bool IsLoggerTopLevelField(const char* key, unsigned long long size)
{
    return memochat::logging::logger_modules::IsTopLevelFieldName(key, size);
}
} // namespace memochat::tests::logging
