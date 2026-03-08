#include "logging/Telemetry.h"

#include "logging/TraceContext.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <json/json.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace memolog {

TelemetryConfig Telemetry::config_{};
std::string Telemetry::service_name_ = "unknown";
std::string Telemetry::service_instance_ = "unknown";

namespace {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

struct ParsedHttpEndpoint {
    bool ok = false;
    std::string host;
    std::string port = "80";
    std::string target = "/";
};

long long NowUnixMicros() {
    using namespace std::chrono;
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

ParsedHttpEndpoint ParseHttpEndpoint(const std::string& raw) {
    ParsedHttpEndpoint endpoint;
    std::string value = raw;
    const std::string http_prefix = "http://";
    if (value.rfind(http_prefix, 0) == 0) {
        value.erase(0, http_prefix.size());
    }

    const auto slash_pos = value.find('/');
    std::string host_port = slash_pos == std::string::npos ? value : value.substr(0, slash_pos);
    endpoint.target = slash_pos == std::string::npos ? "/" : value.substr(slash_pos);
    if (host_port.empty()) {
        return endpoint;
    }

    const auto colon_pos = host_port.rfind(':');
    if (colon_pos != std::string::npos && colon_pos + 1 < host_port.size()) {
        endpoint.host = host_port.substr(0, colon_pos);
        endpoint.port = host_port.substr(colon_pos + 1);
    } else {
        endpoint.host = host_port;
    }

    endpoint.ok = !endpoint.host.empty() && !endpoint.target.empty();
    return endpoint;
}

std::string DetectServiceInstance(const std::string& service_name) {
    const char* host = std::getenv("COMPUTERNAME");
    if (host == nullptr || *host == '\0') {
        host = std::getenv("HOSTNAME");
    }
    const std::string host_name = (host == nullptr || *host == '\0') ? "localhost" : std::string(host);
#ifdef _WIN32
    const int pid = _getpid();
#else
    const int pid = getpid();
#endif
    return service_name + "@" + host_name + ":" + std::to_string(pid);
}

void PostJsonBestEffort(const std::string& endpoint, const std::string& body) {
    const ParsedHttpEndpoint parsed = ParseHttpEndpoint(endpoint);
    if (!parsed.ok) {
        return;
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::seconds(1));
        auto results = resolver.resolve(parsed.host, parsed.port);
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, parsed.target, 11};
        req.set(http::field::host, parsed.host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    } catch (...) {
    }
}

std::string ToJsonPayload(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string out = Json::writeString(builder, value);
    if (!out.empty() && out.back() == '\n') {
        out.pop_back();
    }
    return out;
}

} // namespace

void Telemetry::Init(const std::string& service_name, const TelemetryConfig& cfg) {
    config_ = cfg;
    service_name_ = service_name.empty() ? "unknown" : service_name;
    if (config_.service_name.empty()) {
        config_.service_name = service_name_;
    }
    service_instance_ = DetectServiceInstance(service_name_);
}

void Telemetry::Shutdown() {
}

const TelemetryConfig& Telemetry::Config() {
    return config_;
}

const std::string& Telemetry::ServiceName() {
    return service_name_;
}

const std::string& Telemetry::ServiceInstance() {
    return service_instance_;
}

bool Telemetry::Enabled() {
    return config_.enabled && config_.export_traces;
}

void Telemetry::ExportSpan(const std::string& trace_id,
                           const std::string& span_id,
                           const std::string& parent_span_id,
                           const std::string& name,
                           const std::string& kind,
                           long long start_time_unix_us,
                           long long duration_us,
                           const std::map<std::string, std::string>& attributes) {
    if (!Enabled() || trace_id.empty() || span_id.empty()) {
        return;
    }

    Json::Value span(Json::objectValue);
    span["traceId"] = trace_id;
    span["id"] = span_id;
    if (!parent_span_id.empty()) {
        span["parentId"] = parent_span_id;
    }
    span["name"] = name;
    span["timestamp"] = Json::Int64(start_time_unix_us);
    span["duration"] = Json::Int64(duration_us);
    span["kind"] = kind;

    Json::Value local_endpoint(Json::objectValue);
    local_endpoint["serviceName"] = config_.service_name.empty() ? service_name_ : config_.service_name;
    span["localEndpoint"] = local_endpoint;

    Json::Value tags(Json::objectValue);
    tags["service.instance.id"] = service_instance_;
    tags["service.namespace"] = config_.service_namespace;
    for (const auto& entry : attributes) {
        if (!entry.first.empty() && !entry.second.empty()) {
            tags[entry.first] = entry.second;
        }
    }
    span["tags"] = tags;

    Json::Value payload(Json::arrayValue);
    payload.append(span);
    PostJsonBestEffort(config_.otlp_endpoint, ToJsonPayload(payload));
}

SpanScope::SpanScope(const std::string& name,
                     const std::string& kind,
                     const std::map<std::string, std::string>& attributes)
    : name_(name),
      kind_(kind),
      trace_id_(TraceContext::EnsureTraceId()),
      parent_span_id_(TraceContext::GetSpanId()),
      previous_span_id_(TraceContext::GetSpanId()),
      attributes_(attributes) {
    if (!Telemetry::Enabled() || trace_id_.empty()) {
        return;
    }
    span_id_ = TraceContext::NewSpanId();
    if (span_id_.empty()) {
        return;
    }
    start_time_unix_us_ = NowUnixMicros();
    TraceContext::SetSpanId(span_id_);
    active_ = true;
}

SpanScope::~SpanScope() {
    if (!active_) {
        return;
    }
    const long long end_time_unix_us = NowUnixMicros();
    const long long duration_us = end_time_unix_us > start_time_unix_us_
        ? end_time_unix_us - start_time_unix_us_
        : 0;
    Telemetry::ExportSpan(trace_id_,
                          span_id_,
                          parent_span_id_,
                          name_,
                          kind_,
                          start_time_unix_us_,
                          duration_us,
                          attributes_);
    TraceContext::SetSpanId(previous_span_id_);
}

void SpanScope::SetAttribute(const std::string& key, const std::string& value) {
    if (!key.empty()) {
        attributes_[key] = value;
    }
}

void SpanScope::SetStatusError(const std::string& error_type, const std::string& error_message) {
    attributes_["otel.status_code"] = "ERROR";
    if (!error_type.empty()) {
        attributes_["error.type"] = error_type;
    }
    if (!error_message.empty()) {
        attributes_["error"] = error_message;
    }
}

} // namespace memolog
