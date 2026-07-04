export module memochat.logging.log_config_algorithms;

export namespace memochat::logging::log_config_modules
{
// Exact ASCII literal comparison over an explicit (data,size) view. `lit` is a
// NUL-terminated literal; the view matches only when every byte is equal and
// both ends terminate together.
bool EqualsLiteral(const char* data, unsigned long long size, const char* lit)
{
    unsigned long long index = 0;
    for (; index < size; ++index)
    {
        if (lit[index] == '\0' || data[index] != lit[index])
        {
            return false;
        }
    }
    return lit[size] == '\0';
}

// Truthy tokens accepted by LogConfig::ParseBool after trim+lower.
bool IsTrueBoolToken(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "1") || EqualsLiteral(data, size, "true") ||
           EqualsLiteral(data, size, "yes") || EqualsLiteral(data, size, "on");
}

// Falsy tokens accepted by LogConfig::ParseBool after trim+lower.
bool IsFalseBoolToken(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "0") || EqualsLiteral(data, size, "false") ||
           EqualsLiteral(data, size, "no") || EqualsLiteral(data, size, "off");
}

// Non-positive max_files falls back to the 14-file default.
int NormalizeMaxFiles(int value)
{
    return value <= 0 ? 14 : value;
}

// Non-positive max_size_mb falls back to the 100 MB default.
int NormalizeMaxSizeMb(int value)
{
    return value <= 0 ? 100 : value;
}

bool IsValidRotateMode(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "daily") || EqualsLiteral(data, size, "size");
}

const char* DefaultRotateMode()
{
    return "daily";
}

bool IsValidLogLevel(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "trace") || EqualsLiteral(data, size, "debug") ||
           EqualsLiteral(data, size, "info") || EqualsLiteral(data, size, "warn") ||
           EqualsLiteral(data, size, "error") || EqualsLiteral(data, size, "critical");
}

const char* DefaultLogLevel()
{
    return "info";
}
} // namespace memochat::logging::log_config_modules
