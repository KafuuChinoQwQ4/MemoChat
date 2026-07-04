import memochat.logging.grpc_trace_algorithms;

namespace memochat::tests::logging
{
bool ShouldInjectGrpcTraceId(bool trace_id_empty)
{
    return memochat::logging::grpc_trace_modules::ShouldInjectTraceId(trace_id_empty);
}

bool ShouldGenerateGrpcRequestId(bool request_id_empty)
{
    return memochat::logging::grpc_trace_modules::ShouldGenerateRequestId(request_id_empty);
}

bool ShouldInjectGrpcSpanId(bool span_id_empty)
{
    return memochat::logging::grpc_trace_modules::ShouldInjectSpanId(span_id_empty);
}

bool ShouldGenerateBoundGrpcTraceId(bool metadata_trace_id_empty)
{
    return memochat::logging::grpc_trace_modules::ShouldGenerateBoundTraceId(metadata_trace_id_empty);
}

bool ShouldGenerateBoundGrpcRequestId(bool metadata_request_id_empty)
{
    return memochat::logging::grpc_trace_modules::ShouldGenerateBoundRequestId(metadata_request_id_empty);
}
} // namespace memochat::tests::logging
