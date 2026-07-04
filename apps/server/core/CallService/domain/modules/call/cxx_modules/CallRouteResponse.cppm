export module memochat.call.route_response_algorithms;

export namespace memochat::call::route_response::modules
{
// HTTP status code written on every successful call route response.
int OkStatus()
{
    return 200;
}

// Content type for the JSON POST call routes (start/accept/reject/cancel/hangup/token).
const char* JsonContentType()
{
    return "application/json";
}

// Content type for the legacy GET /api/call/token response.
const char* TokenJsonContentType()
{
    return "text/json";
}

// Needle used to detect a pre-existing trace_id field before appending one.
const char* TraceIdSearchToken()
{
    return "\"trace_id\"";
}
} // namespace memochat::call::route_response::modules
