#pragma once

#include "logging/TelemetryConfig.h"

#include <map>
#include <string>

namespace memolog {

class Telemetry {
public:
    static void Init(const std::string& service_name, const TelemetryConfig& cfg);
    static void Shutdown();
    static const TelemetryConfig& Config();
    static const std::string& ServiceName();
    static const std::string& ServiceInstance();
    static bool Enabled();
    static void ExportSpan(const std::string& trace_id,
                           const std::string& span_id,
                           const std::string& parent_span_id,
                           const std::string& name,
                           const std::string& kind,
                           long long start_time_unix_us,
                           long long duration_us,
                           const std::map<std::string, std::string>& attributes);

private:
    static TelemetryConfig config_;
    static std::string service_name_;
    static std::string service_instance_;
};

class SpanScope {
public:
    SpanScope(const std::string& name,
              const std::string& kind,
              const std::map<std::string, std::string>& attributes = {});
    ~SpanScope();

    void SetAttribute(const std::string& key, const std::string& value);
    void SetStatusError(const std::string& error_type, const std::string& error_message = "");

private:
    bool active_ = false;
    std::string name_;
    std::string kind_;
    std::string trace_id_;
    std::string span_id_;
    std::string parent_span_id_;
    std::string previous_span_id_;
    long long start_time_unix_us_ = 0;
    std::map<std::string, std::string> attributes_;
};

} // namespace memolog
