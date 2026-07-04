export module memochat.gate.h3_route_adapter_algorithms;

// GateServerHttp3 H3 route adapter guard/metadata decisions.
// H3RouteAdapter bridges a GateHttp3Connection to the shared RouteRegistry.
// This module owns the adapter's primitive guard decisions, default content
// types, and the file-not-found status. It exports bools, ints, and const
// char* literals only; it does NOT export GateHttp3Connection, GateRequest,
// GateResponse, RouteRegistry, TraceContext, std::string, std::optional,
// file I/O, query parsing, or any Boost/Beast/HTTP3 type.
export namespace memochat::gate::adapters::h3::modules
{
// Both BuildGateRequest and ApplyGateResponse and Dispatch early-return unless
// a live connection pointer exists.
bool ShouldProcessConnection(bool has_connection)
{
    return has_connection;
}

// GateRequest.target uses "path?query" only when the query string is non-empty,
// otherwise it is just the path.
bool ShouldAppendQueryToTarget(bool query_empty)
{
    return !query_empty;
}

// File-body response reads the file; when the file body is missing the adapter
// responds with 404 and this JSON error payload.
int FileBodyNotFoundStatus()
{
    return 404;
}

const char* FileBodyNotFoundBody()
{
    return R"({"error":404,"message":"file response body not found"})";
}

const char* FileBodyNotFoundContentType()
{
    return "application/json";
}

// File responses default to octet-stream when the route left content type empty.
const char* DefaultFileContentType()
{
    return "application/octet-stream";
}

// Inline responses default to JSON when the route left content type empty.
const char* DefaultInlineContentType()
{
    return "application/json";
}

const char* ResolveContentType(bool content_type_empty, const char* default_content_type, const char* route_content_type)
{
    return content_type_empty ? default_content_type : route_content_type;
}
} // namespace memochat::gate::adapters::h3::modules
