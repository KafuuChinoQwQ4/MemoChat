#include "logging/TelemetryConfig.hpp"

#include <charconv>

import memochat.logging.config_algorithms;
import memochat.logging.telemetry_config_algorithms;

namespace memolog
{
namespace telemetry_config_modules = memochat::logging::telemetry_config_modules;

namespace
{

std::string Trim(std::string value)
{
    const auto begin = memochat::logging::modules::TrimAsciiBegin(value.data(), value.size());
    const auto end = memochat::logging::modules::TrimAsciiEnd(value.data(), value.size());
    return value.substr(begin, end - begin);
}

std::string ToLower(std::string value)
{
    memochat::logging::modules::LowerAsciiInPlace(value.data(), value.size());
    return value;
}

bool ParseBool(const std::string& raw, bool fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    const std::string v = ToLower(Trim(raw));
    if (telemetry_config_modules::IsTrueBoolToken(v.data(), v.size()))
    {
        return true;
    }
    if (telemetry_config_modules::IsFalseBoolToken(v.data(), v.size()))
    {
        return false;
    }
    return fallback;
}

double ParseDouble(const std::string& raw, double fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    double value = 0.0;
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == raw.data() + raw.size() ? value : fallback;
}

} // namespace

TelemetryConfig
TelemetryConfig::FromGetter(const std::function<std::string(const std::string&, const std::string&)>& getter)
{
    TelemetryConfig cfg;
    if (!getter)
    {
        return cfg;
    }

    const auto read = [&](const std::string& key, const std::string& fallback)
    {
        const std::string value = Trim(getter("Telemetry", key));
        return value.empty() ? fallback : value;
    };

    cfg.enabled = ParseBool(read("Enabled", cfg.enabled ? "true" : "false"), cfg.enabled);
    cfg.otlp_endpoint = read("OtlpEndpoint", cfg.otlp_endpoint);
    cfg.protocol = ToLower(read("Protocol", cfg.protocol));
    cfg.sample_ratio = ParseDouble(read("SampleRatio", "1.0"), cfg.sample_ratio);
    cfg.export_logs = ParseBool(read("ExportLogs", cfg.export_logs ? "true" : "false"), cfg.export_logs);
    cfg.export_traces = ParseBool(read("ExportTraces", cfg.export_traces ? "true" : "false"), cfg.export_traces);
    cfg.export_metrics = ParseBool(read("ExportMetrics", cfg.export_metrics ? "true" : "false"), cfg.export_metrics);
    cfg.service_name = read("ServiceName", cfg.service_name);
    cfg.service_namespace = read("ServiceNamespace", cfg.service_namespace);

    cfg.sample_ratio = telemetry_config_modules::ClampSampleRatio(cfg.sample_ratio);

    if (telemetry_config_modules::ShouldUseDefaultProtocol(cfg.protocol.empty()))
    {
        cfg.protocol = telemetry_config_modules::DefaultProtocol();
    }
    return cfg;
}

} // namespace memolog
