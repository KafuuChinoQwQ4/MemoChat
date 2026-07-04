export module memochat.logging.grpc_trace_algorithms;

export namespace memochat::logging::grpc_trace_modules
{
bool ShouldInjectTraceId(bool trace_id_empty)
{
    return !trace_id_empty;
}

bool ShouldGenerateRequestId(bool request_id_empty)
{
    return request_id_empty;
}

bool ShouldInjectSpanId(bool span_id_empty)
{
    return !span_id_empty;
}

bool ShouldGenerateBoundTraceId(bool metadata_trace_id_empty)
{
    return metadata_trace_id_empty;
}

bool ShouldGenerateBoundRequestId(bool metadata_request_id_empty)
{
    return metadata_request_id_empty;
}
} // namespace memochat::logging::grpc_trace_modules
