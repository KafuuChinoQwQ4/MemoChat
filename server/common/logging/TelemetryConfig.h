#pragma once

#include <functional>
#include <string>

namespace memolog {

struct TelemetryConfig {
    bool enabled = false;
    std::string otlp_endpoint = "http://127.0.0.1:9411/api/v2/spans";
    std::string protocol = "zipkin-json";
    double sample_ratio = 1.0;
    bool export_logs = true;
    bool export_traces = true;
    bool export_metrics = false;
    std::string service_name;
    std::string service_namespace = "memochat";

    static TelemetryConfig FromGetter(
        const std::function<std::string(const std::string&, const std::string&)>& getter);
};

} // namespace memolog
