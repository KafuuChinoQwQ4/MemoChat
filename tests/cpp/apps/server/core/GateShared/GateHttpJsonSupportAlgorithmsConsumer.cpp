import memochat.gate.http_json_support_algorithms;

namespace memochat::tests::gate::http_json
{
const char* JsonContentType()
{
    return memochat::gate::http_json::modules::JsonContentType();
}

const char* EmptyTraceId()
{
    return memochat::gate::http_json::modules::EmptyTraceId();
}

bool ShouldWriteJsonParseError(bool parsed_json)
{
    return memochat::gate::http_json::modules::ShouldWriteJsonParseError(parsed_json);
}

bool ShouldAttachTraceId(bool parsed_json)
{
    return memochat::gate::http_json::modules::ShouldAttachTraceId(parsed_json);
}
} // namespace memochat::tests::gate::http_json
