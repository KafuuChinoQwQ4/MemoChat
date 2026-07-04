#include "CallPublicDtos.hpp"

#include "json/TypedJsonCodec.hpp"

#include <exception>

import memochat.call.public_dto_algorithms;

namespace
{

bool ParseJsonForCallPublic(std::string_view body, memochat::json::JsonValue* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    if (!memochat::json::glaze_parse(*out, body))
    {
        if (error_out != nullptr)
        {
            *error_out = "invalid json";
        }
        return false;
    }
    return true;
}

template <typename T> bool WriteTypedJsonNoThrow(const T& value, std::string* out, std::string* error_out)
{
    try
    {
        return memochat::json::WriteTypedJson(value, out, error_out);
    }
    catch (const std::exception& e)
    {
        if (error_out != nullptr)
        {
            *error_out = e.what();
        }
        return false;
    }
}

template <typename T> memochat::json::JsonValue TypedJsonToJsonValue(const T& value)
{
    std::string body;
    if (!WriteTypedJsonNoThrow(value, &body, nullptr))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }

    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return root;
}

} // namespace

namespace memochat::call
{

CallAuthRequestDto CallAuthRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    CallAuthRequestDto request;
    if (public_dto::modules::ShouldReadOptionalInt(root.isMember("uid")))
    {
        request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    }
    if (public_dto::modules::ShouldReadOptionalText(root.isMember("token")))
    {
        request.token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    }
    if (public_dto::modules::ShouldReadOptionalText(root.isMember("call_id")))
    {
        request.call_id = memochat::json::glaze_safe_get<std::string>(root, "call_id", "");
    }
    return request;
}

CallStartRequestDto CallStartRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    CallStartRequestDto request;
    if (public_dto::modules::ShouldReadOptionalInt(root.isMember("uid")))
    {
        request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    }
    if (public_dto::modules::ShouldReadOptionalText(root.isMember("token")))
    {
        request.token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    }
    if (public_dto::modules::ShouldReadOptionalInt(root.isMember("peer_uid")))
    {
        request.peer_uid = memochat::json::glaze_safe_get<int>(root, "peer_uid", 0);
    }
    if (public_dto::modules::ShouldReadOptionalText(root.isMember("call_type")))
    {
        request.call_type = memochat::json::glaze_safe_get<std::string>(root, "call_type", "");
    }
    return request;
}

CallTokenRequestDto CallTokenRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    CallTokenRequestDto request;
    if (public_dto::modules::ShouldReadOptionalInt(root.isMember("uid")))
    {
        request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    }
    if (public_dto::modules::ShouldReadOptionalText(root.isMember("token")))
    {
        request.token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    }
    if (public_dto::modules::ShouldReadOptionalText(root.isMember("call_id")))
    {
        request.call_id = memochat::json::glaze_safe_get<std::string>(root, "call_id", "");
    }
    if (public_dto::modules::ShouldReadOptionalText(root.isMember("role")))
    {
        request.role = memochat::json::glaze_safe_get<std::string>(root, "role", "");
    }
    return request;
}

memochat::json::JsonValue CallEventResponseToJsonValue(const CallEventResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue CallStartResponseToJsonValue(const CallStartResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue CallTokenResponseToJsonValue(const CallTokenResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

bool DecodeCallAuthRequest(std::string_view body, CallAuthRequestDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    memochat::json::JsonValue root;
    if (!ParseJsonForCallPublic(body, &root, error_out))
    {
        return false;
    }
    *out = CallAuthRequestFromJsonValue(root);
    return true;
}

bool DecodeCallStartRequest(std::string_view body, CallStartRequestDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    memochat::json::JsonValue root;
    if (!ParseJsonForCallPublic(body, &root, error_out))
    {
        return false;
    }
    *out = CallStartRequestFromJsonValue(root);
    return true;
}

bool DecodeCallTokenRequest(std::string_view body, CallTokenRequestDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    memochat::json::JsonValue root;
    if (!ParseJsonForCallPublic(body, &root, error_out))
    {
        return false;
    }
    *out = CallTokenRequestFromJsonValue(root);
    return true;
}

} // namespace memochat::call
