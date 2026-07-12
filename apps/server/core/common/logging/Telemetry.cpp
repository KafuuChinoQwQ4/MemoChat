#include "logging/Telemetry.hpp"

#include "logging/TraceContext.hpp"
#include "runtime/ExplicitThread.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "json/GlazeCompat.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <string>

import memochat.logging.telemetry_algorithms;

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace memolog
{

TelemetryConfig Telemetry::config_{};
std::string Telemetry::service_name_ = "unknown";
std::string Telemetry::service_instance_ = "unknown";

namespace
{
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

struct ParsedHttpEndpoint
{
    bool ok = false;
    std::string host;
    std::string port = "80";
    std::string target = "/";
};

std::mutex g_export_mutex;
std::condition_variable g_export_cv;
std::deque<std::pair<std::string, std::string>> g_export_queue;
memochat::runtime::ExplicitThread g_export_thread;
bool g_export_stop = false;
bool g_export_started = false;
constexpr std::size_t kMaxQueuedExports = 1024;

long long NowUnixMicros()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

ParsedHttpEndpoint ParseHttpEndpoint(const std::string& raw)
{
    ParsedHttpEndpoint endpoint;
    std::string value = raw;
    if (memochat::logging::telemetry_modules::HasHttpSchemePrefix(value.data(), value.size()))
    {
        value.erase(0, memochat::logging::telemetry_modules::HttpSchemePrefixSize());
    }

    const auto slash_pos = value.find('/');
    std::string host_port = slash_pos == std::string::npos ? value : value.substr(0, slash_pos);
    endpoint.target = slash_pos == std::string::npos ? "/" : value.substr(slash_pos);
    if (!memochat::logging::telemetry_modules::HasUsableEndpointAuthority(host_port.empty()))
    {
        return endpoint;
    }

    const auto colon_pos = host_port.rfind(':');
    if (memochat::logging::telemetry_modules::HasExplicitEndpointPort(colon_pos != std::string::npos,
                                                                      colon_pos,
                                                                      host_port.size()))
    {
        endpoint.host = host_port.substr(0, colon_pos);
        endpoint.port = host_port.substr(colon_pos + 1);
    }
    else
    {
        endpoint.host = host_port;
    }

    endpoint.ok =
        memochat::logging::telemetry_modules::IsParsedHttpEndpointOk(endpoint.host.empty(), endpoint.target.empty());
    return endpoint;
}

std::string DetectServiceInstance(const std::string& service_name)
{
    const char* host = std::getenv("COMPUTERNAME");
    if (host == nullptr || *host == '\0')
    {
        host = std::getenv("HOSTNAME");
    }
    const char* instance = std::getenv("MEMOCHAT_INSTANCE_NAME");
    const bool host_empty = host == nullptr || *host == '\0';
    const bool instance_empty = instance == nullptr || *instance == '\0';
    const std::string host_name =
        memochat::logging::telemetry_modules::ShouldUseFallbackText(host_empty) ? "localhost" : std::string(host);
    const std::string instance_name =
        memochat::logging::telemetry_modules::ShouldUseFallbackText(instance_empty) ? "" : std::string(instance);
#ifdef _WIN32
    const int pid = _getpid();
#else
    const int pid = getpid();
#endif
    std::string value = service_name + "@";
    if (memochat::logging::telemetry_modules::ShouldIncludeInstanceName(instance_name.empty()))
    {
        value += instance_name + ".";
    }
    value += host_name + ":" + std::to_string(pid);
    return value;
}

void PostJsonBestEffort(const std::string& endpoint, const std::string& body)
{
    const ParsedHttpEndpoint parsed = ParseHttpEndpoint(endpoint);
    if (!parsed.ok)
    {
        return;
    }

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);
    stream.expires_after(std::chrono::seconds(1));
    beast::error_code ec;
    auto results = resolver.resolve(parsed.host, parsed.port, ec);
    if (ec)
    {
        return;
    }
    stream.connect(results, ec);
    if (ec)
    {
        return;
    }

    http::request<http::string_body> req{http::verb::post, parsed.target, 11};
    req.set(http::field::host, parsed.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();

    http::write(stream, req, ec);
    if (ec)
    {
        return;
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);
    if (ec)
    {
        return;
    }

    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
}

std::string ToJsonPayload(const memochat::json::JsonValue& value)
{
    return memochat::json::glaze_stringify(value);
}

void ExportWorkerLoop()
{
    for (;;)
    {
        std::pair<std::string, std::string> job;
        {
            std::unique_lock<std::mutex> lock(g_export_mutex);
            g_export_cv.wait(lock,
                             []()
                             {
                                 return g_export_stop || !g_export_queue.empty();
                             });
            if (g_export_stop && g_export_queue.empty())
            {
                return;
            }
            job = std::move(g_export_queue.front());
            g_export_queue.pop_front();
        }
        PostJsonBestEffort(job.first, job.second);
    }
}

bool StartExportWorkerIfNeeded()
{
    std::lock_guard<std::mutex> lock(g_export_mutex);
    if (g_export_started)
    {
        return true;
    }
    g_export_stop = false;
    std::string error;
    if (!g_export_thread.Start(
            []() noexcept
            {
                ExportWorkerLoop();
            },
            &error))
    {
        std::fprintf(stderr, "[Telemetry] Failed to start exporter thread: %s\n", error.c_str());
        return false;
    }
    g_export_started = true;
    return true;
}

void StopExportWorker()
{
    {
        std::lock_guard<std::mutex> lock(g_export_mutex);
        if (!g_export_started)
        {
            return;
        }
        g_export_stop = true;
    }
    g_export_cv.notify_all();
    if (g_export_thread.Joinable())
    {
        std::string error;
        if (!g_export_thread.Join(&error))
        {
            std::fprintf(stderr, "[Telemetry] Failed to join exporter thread: %s\n", error.c_str());
        }
    }
    std::lock_guard<std::mutex> lock(g_export_mutex);
    g_export_queue.clear();
    g_export_started = false;
    g_export_stop = false;
}

struct ExportWorkerShutdown
{
    ~ExportWorkerShutdown()
    {
        StopExportWorker();
    }
};

ExportWorkerShutdown g_export_worker_shutdown;

void EnqueueJsonBestEffort(const std::string& endpoint, const std::string& body)
{
    if (memochat::logging::telemetry_modules::ShouldDropExportJob(endpoint.empty(), body.empty()))
    {
        return;
    }
    if (!StartExportWorkerIfNeeded())
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_export_mutex);
        if (g_export_queue.size() >= kMaxQueuedExports)
        {
            g_export_queue.pop_front();
        }
        g_export_queue.emplace_back(endpoint, body);
    }
    g_export_cv.notify_one();
}

} // namespace

void Telemetry::Init(const std::string& service_name, const TelemetryConfig& cfg)
{
    config_ = cfg;
    service_name_ =
        memochat::logging::telemetry_modules::ShouldUseFallbackText(service_name.empty()) ? "unknown" : service_name;
    if (memochat::logging::telemetry_modules::ShouldUseFallbackText(config_.service_name.empty()))
    {
        config_.service_name = service_name_;
    }
    service_instance_ = DetectServiceInstance(service_name_);
    if (Enabled())
    {
        StartExportWorkerIfNeeded();
    }
}

void Telemetry::Shutdown()
{
    StopExportWorker();
}

const TelemetryConfig& Telemetry::Config()
{
    return config_;
}

const std::string& Telemetry::ServiceName()
{
    return service_name_;
}

const std::string& Telemetry::ServiceInstance()
{
    return service_instance_;
}

bool Telemetry::Enabled()
{
    return memochat::logging::telemetry_modules::IsTelemetryEnabled(config_.enabled, config_.export_traces);
}

void Telemetry::ExportSpan(const std::string& trace_id,
                           const std::string& span_id,
                           const std::string& parent_span_id,
                           const std::string& name,
                           const std::string& kind,
                           long long start_time_unix_us,
                           long long duration_us,
                           const std::map<std::string, std::string>& attributes)
{
    if (!Enabled() || trace_id.empty() || span_id.empty())
    {
        return;
    }

    memochat::json::JsonValue span = memochat::json::glaze_empty_object();
    span["traceId"] = trace_id;
    span["id"] = span_id;
    if (!parent_span_id.empty())
    {
        span["parentId"] = parent_span_id;
    }
    span["name"] = name;
    span["timestamp"] = static_cast<double>(start_time_unix_us);
    span["duration"] = static_cast<double>(duration_us);
    span["kind"] = kind;

    memochat::json::JsonValue local_endpoint = memochat::json::glaze_empty_object();
    local_endpoint["serviceName"] = config_.service_name.empty() ? service_name_ : config_.service_name;
    span["localEndpoint"] = local_endpoint;

    memochat::json::JsonValue tags = memochat::json::glaze_empty_object();
    tags["service.instance.id"] = service_instance_;
    tags["service.namespace"] = config_.service_namespace;
    for (const auto& entry : attributes)
    {
        if (memochat::logging::telemetry_modules::ShouldKeepSpanAttribute(entry.first.empty(), entry.second.empty()))
        {
            tags[entry.first] = entry.second;
        }
    }
    span["tags"] = tags;

    memochat::json::JsonValue payload = memochat::json::glaze_empty_array();
    memochat::json::append(payload, span);
    EnqueueJsonBestEffort(config_.otlp_endpoint, ToJsonPayload(payload));
}

SpanScope::SpanScope(const std::string& name,
                     const std::string& kind,
                     const std::map<std::string, std::string>& attributes)
    : name_(name)
    , kind_(kind)
    , trace_id_(TraceContext::EnsureTraceId())
    , parent_span_id_(TraceContext::GetSpanId())
    , previous_span_id_(TraceContext::GetSpanId())
    , attributes_(attributes)
{
    if (!Telemetry::Enabled() || trace_id_.empty())
    {
        return;
    }
    span_id_ = TraceContext::NewSpanId();
    if (span_id_.empty())
    {
        return;
    }
    start_time_unix_us_ = NowUnixMicros();
    TraceContext::SetSpanId(span_id_);
    active_ = true;
}

SpanScope::~SpanScope()
{
    if (!active_)
    {
        return;
    }
    const long long end_time_unix_us = NowUnixMicros();
    const long long duration_us = end_time_unix_us > start_time_unix_us_ ? end_time_unix_us - start_time_unix_us_ : 0;
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

void SpanScope::SetAttribute(const std::string& key, const std::string& value)
{
    if (!key.empty())
    {
        attributes_[key] = value;
    }
}

void SpanScope::SetStatusError(const std::string& error_type, const std::string& error_message)
{
    attributes_["otel.status_code"] = "ERROR";
    if (!error_type.empty())
    {
        attributes_["error.type"] = error_type;
    }
    if (!error_message.empty())
    {
        attributes_["error"] = error_message;
    }
}

} // namespace memolog
