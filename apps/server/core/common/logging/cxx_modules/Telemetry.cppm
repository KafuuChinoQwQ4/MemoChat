export module memochat.logging.telemetry_algorithms;

export namespace memochat::logging::telemetry_modules
{
bool HasHttpSchemePrefix(const char* data, unsigned long long size)
{
    constexpr char kPrefix[] = "http://";
    constexpr unsigned long long kPrefixSize = 7;
    if (data == nullptr || size < kPrefixSize)
    {
        return false;
    }
    for (unsigned long long index = 0; index < kPrefixSize; ++index)
    {
        if (data[index] != kPrefix[index])
        {
            return false;
        }
    }
    return true;
}

unsigned long long HttpSchemePrefixSize()
{
    return 7;
}

bool HasUsableEndpointAuthority(bool authority_empty)
{
    return !authority_empty;
}

bool HasExplicitEndpointPort(bool colon_found, unsigned long long colon_pos, unsigned long long authority_size)
{
    return colon_found && colon_pos + 1 < authority_size;
}

bool IsParsedHttpEndpointOk(bool host_empty, bool target_empty)
{
    return !host_empty && !target_empty;
}

bool ShouldUseFallbackText(bool text_empty)
{
    return text_empty;
}

bool ShouldIncludeInstanceName(bool instance_name_empty)
{
    return !instance_name_empty;
}

bool IsTelemetryEnabled(bool enabled, bool export_traces)
{
    return enabled && export_traces;
}

bool ShouldDropExportJob(bool endpoint_empty, bool body_empty)
{
    return endpoint_empty || body_empty;
}

bool ShouldKeepSpanAttribute(bool key_empty, bool value_empty)
{
    return !key_empty && !value_empty;
}
} // namespace memochat::logging::telemetry_modules
