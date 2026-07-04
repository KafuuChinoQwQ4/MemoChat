import memochat.logging.telemetry_algorithms;

namespace memochat::tests::logging
{
bool HasHttpSchemePrefix(const char* data, unsigned long long size)
{
    return memochat::logging::telemetry_modules::HasHttpSchemePrefix(data, size);
}

bool HasExplicitEndpointPort(bool colon_found, unsigned long long colon_pos, unsigned long long authority_size)
{
    return memochat::logging::telemetry_modules::HasExplicitEndpointPort(colon_found, colon_pos, authority_size);
}

bool IsParsedHttpEndpointOk(bool host_empty, bool target_empty)
{
    return memochat::logging::telemetry_modules::IsParsedHttpEndpointOk(host_empty, target_empty);
}

bool ShouldUseFallbackTelemetryText(bool text_empty)
{
    return memochat::logging::telemetry_modules::ShouldUseFallbackText(text_empty);
}

bool IsTelemetryEnabled(bool enabled, bool export_traces)
{
    return memochat::logging::telemetry_modules::IsTelemetryEnabled(enabled, export_traces);
}

bool ShouldDropExportJob(bool endpoint_empty, bool body_empty)
{
    return memochat::logging::telemetry_modules::ShouldDropExportJob(endpoint_empty, body_empty);
}

bool ShouldKeepSpanAttribute(bool key_empty, bool value_empty)
{
    return memochat::logging::telemetry_modules::ShouldKeepSpanAttribute(key_empty, value_empty);
}
} // namespace memochat::tests::logging
