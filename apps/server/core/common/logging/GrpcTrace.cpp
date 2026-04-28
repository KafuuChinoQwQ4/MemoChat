#include "logging/GrpcTrace.h"

#include "logging/TraceContext.h"

namespace memolog {
namespace {

std::string ReadMetadata(grpc::ServerContext* context, const char* key) {
    if (context == nullptr) {
        return "";
    }
    const auto it = context->client_metadata().find(key);
    if (it == context->client_metadata().end()) {
        return "";
    }
    return std::string(it->second.data(), it->second.length());
}

} // namespace

void InjectGrpcTraceMetadata(grpc::ClientContext& context) {
    const std::string trace_id = TraceContext::EnsureTraceId();
    if (!trace_id.empty()) {
        context.AddMetadata("x-trace-id", trace_id);
    }
    std::string request_id = TraceContext::GetRequestId();
    if (request_id.empty()) {
        request_id = TraceContext::NewId();
        TraceContext::SetRequestId(request_id);
    }
    context.AddMetadata("x-request-id", request_id);
    const std::string span_id = TraceContext::GetSpanId();
    if (!span_id.empty()) {
        context.AddMetadata("x-span-id", span_id);
    }
}

void BindGrpcTraceContext(grpc::ServerContext* context) {
    std::string trace_id = ReadMetadata(context, "x-trace-id");
    if (trace_id.empty()) {
        trace_id = TraceContext::NewId();
    }
    std::string request_id = ReadMetadata(context, "x-request-id");
    if (request_id.empty()) {
        request_id = TraceContext::NewId();
    }
    TraceContext::SetTraceId(trace_id);
    TraceContext::SetRequestId(request_id);
    TraceContext::SetSpanId(ReadMetadata(context, "x-span-id"));
}

} // namespace memolog
