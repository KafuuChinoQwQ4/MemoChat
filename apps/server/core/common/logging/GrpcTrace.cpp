#include "logging/GrpcTrace.hpp"

#include "logging/TraceContext.hpp"

import memochat.logging.grpc_trace_algorithms;

namespace memolog
{
namespace
{

std::string ReadMetadata(grpc::ServerContext* context, const char* key)
{
    if (context == nullptr)
    {
        return "";
    }
    const auto it = context->client_metadata().find(key);
    if (it == context->client_metadata().end())
    {
        return "";
    }
    return std::string(it->second.data(), it->second.length());
}

} // namespace

void InjectGrpcTraceMetadata(grpc::ClientContext& context)
{
    const std::string trace_id = TraceContext::EnsureTraceId();
    if (memochat::logging::grpc_trace_modules::ShouldInjectTraceId(trace_id.empty()))
    {
        context.AddMetadata("x-trace-id", trace_id);
    }
    std::string request_id = TraceContext::GetRequestId();
    if (memochat::logging::grpc_trace_modules::ShouldGenerateRequestId(request_id.empty()))
    {
        request_id = TraceContext::NewId();
        TraceContext::SetRequestId(request_id);
    }
    context.AddMetadata("x-request-id", request_id);
    const std::string span_id = TraceContext::GetSpanId();
    if (memochat::logging::grpc_trace_modules::ShouldInjectSpanId(span_id.empty()))
    {
        context.AddMetadata("x-span-id", span_id);
    }
}

void BindGrpcTraceContext(grpc::ServerContext* context)
{
    std::string trace_id = ReadMetadata(context, "x-trace-id");
    if (memochat::logging::grpc_trace_modules::ShouldGenerateBoundTraceId(trace_id.empty()))
    {
        trace_id = TraceContext::NewId();
    }
    std::string request_id = ReadMetadata(context, "x-request-id");
    if (memochat::logging::grpc_trace_modules::ShouldGenerateBoundRequestId(request_id.empty()))
    {
        request_id = TraceContext::NewId();
    }
    TraceContext::SetTraceId(trace_id);
    TraceContext::SetRequestId(request_id);
    TraceContext::SetSpanId(ReadMetadata(context, "x-span-id"));
}

} // namespace memolog
