import memochat.logging.log_config_algorithms;

namespace memochat::tests::logging::log_config
{
bool IsTrueBoolToken(const char* data, unsigned long long size)
{
    return memochat::logging::log_config_modules::IsTrueBoolToken(data, size);
}

bool IsFalseBoolToken(const char* data, unsigned long long size)
{
    return memochat::logging::log_config_modules::IsFalseBoolToken(data, size);
}

int NormalizeMaxFiles(int value)
{
    return memochat::logging::log_config_modules::NormalizeMaxFiles(value);
}

int NormalizeMaxSizeMb(int value)
{
    return memochat::logging::log_config_modules::NormalizeMaxSizeMb(value);
}

bool IsValidRotateMode(const char* data, unsigned long long size)
{
    return memochat::logging::log_config_modules::IsValidRotateMode(data, size);
}

const char* DefaultRotateMode()
{
    return memochat::logging::log_config_modules::DefaultRotateMode();
}

bool IsValidLogLevel(const char* data, unsigned long long size)
{
    return memochat::logging::log_config_modules::IsValidLogLevel(data, size);
}

const char* DefaultLogLevel()
{
    return memochat::logging::log_config_modules::DefaultLogLevel();
}
} // namespace memochat::tests::logging::log_config
