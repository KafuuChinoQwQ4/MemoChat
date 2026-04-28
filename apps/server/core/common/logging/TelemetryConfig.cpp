#include "logging/TelemetryConfig.h"

#include <algorithm>
#include <cctype>

namespace memolog {
namespace {

std::string Trim(std::string value) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char c) {
        return !is_space(static_cast<unsigned char>(c));
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](char c) {
        return !is_space(static_cast<unsigned char>(c));
    }).base(), value.end());
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ParseBool(const std::string& raw, bool fallback) {
    if (raw.empty()) {
        return fallback;
    }
    const std::string v = ToLower(Trim(raw));
    if (v == "1" || v == "true" || v == "yes" || v == "on") {
        return true;
    }
    if (v == "0" || v == "false" || v == "no" || v == "off") {
        return false;
    }
    return fallback;
}

double ParseDouble(const std::string& raw, double fallback) {
    try {
        if (!raw.empty()) {
            return std::stod(raw);
        }
    } catch (...) {
    }
    return fallback;
}

} // namespace

TelemetryConfig TelemetryConfig::FromGetter(
    const std::function<std::string(const std::string&, const std::string&)>& getter) {
    TelemetryConfig cfg;
    if (!getter) {
        return cfg;
    }

    const auto read = [&](const std::string& key, const std::string& fallback) {
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

    if (cfg.sample_ratio < 0.0) {
        cfg.sample_ratio = 0.0;
    } else if (cfg.sample_ratio > 1.0) {
        cfg.sample_ratio = 1.0;
    }

    if (cfg.protocol.empty()) {
        cfg.protocol = "zipkin-json";
    }
    return cfg;
}

} // namespace memolog
