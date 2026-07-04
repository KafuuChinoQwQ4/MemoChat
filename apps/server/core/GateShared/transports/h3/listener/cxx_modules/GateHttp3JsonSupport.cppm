export module memochat.gate.h3_json_support_algorithms;

// GateServerHttp3 HTTP/3 JSON POST support guard/metadata decisions.
// GateHttp3JsonSupport parses a request body as JSON and runs a handler,
// producing standard success/error envelopes. This module owns the primitive
// guard decisions, status codes, content type, and stable error field/message
// literals. It exports bools, ints, and const char* literals only; it does NOT
// export GateHttp3Connection, JsonValue, glaze parse/writeString, std::string,
// std::function handlers, or exception objects.
export namespace memochat::gate::h3::json_support::modules
{
// ParseJsonBody rejects a null connection with this message.
const char* NullConnectionMessage()
{
    return "null connection";
}

// ParseJsonBody rejects an empty request body with this message.
const char* EmptyBodyMessage()
{
    return "empty request body";
}

// JSON parse failures prefix the glaze parse error with this text.
const char* JsonParseErrorPrefix()
{
    return "json parse error: ";
}

const char* ResponseContentType()
{
    return "application/json";
}

// Error envelope error code value on failure.
int ErrorCode()
{
    return 1;
}

// Success envelope error code value.
int SuccessCode()
{
    return 0;
}

// Parse failures and handler-returned false both respond 400.
int BadRequestStatus()
{
    return 400;
}

// Handler success responds 200.
int OkStatus()
{
    return 200;
}

// Exceptions thrown by the handler respond 500 with a rebuilt envelope.
int InternalErrorStatus()
{
    return 500;
}

// When the handler returns false, the error field is forced to ErrorCode()
// only if it is missing or still zero.
bool ShouldForceErrorCode(bool has_error_member, int current_error)
{
    return !has_error_member || current_error == 0;
}
} // namespace memochat::gate::h3::json_support::modules
