import memochat.logging.telemetry_config_algorithms;

namespace memochat::tests::logging::telemetry_config
{
bool IsTrueBoolToken(const char* data, unsigned long long size)
{
    return memochat::logging::telemetry_config_modules::IsTrueBoolToken(data, size);
}

bool IsFalseBoolToken(const char* data, unsigned long long size)
{
    return memochat::logging::telemetry_config_modules::IsFalseBoolToken(data, size);
}

double ClampSampleRatio(double value)
{
    return memochat::logging::telemetry_config_modules::ClampSampleRatio(value);
}

bool ShouldUseDefaultProtocol(bool protocol_empty)
{
    return memochat::logging::telemetry_config_modules::ShouldUseDefaultProtocol(protocol_empty);
}

const char* DefaultProtocol()
{
    return memochat::logging::telemetry_config_modules::DefaultProtocol();
}
} // namespace memochat::tests::logging::telemetry_config
