import memochat.call.route_response_algorithms;

namespace memochat::tests::call::route_response
{
int OkStatus()
{
    return memochat::call::route_response::modules::OkStatus();
}

const char* JsonContentType()
{
    return memochat::call::route_response::modules::JsonContentType();
}

const char* TokenJsonContentType()
{
    return memochat::call::route_response::modules::TokenJsonContentType();
}

const char* TraceIdSearchToken()
{
    return memochat::call::route_response::modules::TraceIdSearchToken();
}
} // namespace memochat::tests::call::route_response
