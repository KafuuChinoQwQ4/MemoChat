export module memochat.logging.telemetry_config_algorithms;

export namespace memochat::logging::telemetry_config_modules
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

// Truthy tokens accepted by TelemetryConfig::ParseBool after trim+lower.
bool IsTrueBoolToken(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "1") || EqualsLiteral(data, size, "true") ||
           EqualsLiteral(data, size, "yes") || EqualsLiteral(data, size, "on");
}

// Falsy tokens accepted by TelemetryConfig::ParseBool after trim+lower.
bool IsFalseBoolToken(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "0") || EqualsLiteral(data, size, "false") ||
           EqualsLiteral(data, size, "no") || EqualsLiteral(data, size, "off");
}

// sample_ratio is clamped to the inclusive [0.0, 1.0] range.
double ClampSampleRatio(double value)
{
    if (value < 0.0)
    {
        return 0.0;
    }
    if (value > 1.0)
    {
        return 1.0;
    }
    return value;
}

// An empty protocol falls back to the default transport encoding.
bool ShouldUseDefaultProtocol(bool protocol_empty)
{
    return protocol_empty;
}

const char* DefaultProtocol()
{
    return "zipkin-json";
}
} // namespace memochat::logging::telemetry_config_modules
