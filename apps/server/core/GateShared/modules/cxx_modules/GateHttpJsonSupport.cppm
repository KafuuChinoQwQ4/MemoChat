export module memochat.gate.http_json_support_algorithms;

export namespace memochat::gate::http_json::modules
{
const char* JsonContentType()
{
    return "application/json";
}

const char* EmptyTraceId()
{
    return "";
}

bool ShouldWriteJsonParseError(bool parsed_json)
{
    return !parsed_json;
}

bool ShouldAttachTraceId(bool parsed_json)
{
    return parsed_json;
}
} // namespace memochat::gate::http_json::modules
