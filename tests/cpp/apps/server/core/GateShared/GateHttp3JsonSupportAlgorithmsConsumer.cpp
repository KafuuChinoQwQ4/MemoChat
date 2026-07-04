#include <string>

import memochat.gate.h3_json_support_algorithms;

namespace memochat::tests::gate::h3_json_support
{
std::string NullConnectionMessage()
{
    return memochat::gate::h3::json_support::modules::NullConnectionMessage();
}

std::string EmptyBodyMessage()
{
    return memochat::gate::h3::json_support::modules::EmptyBodyMessage();
}

std::string JsonParseErrorPrefix()
{
    return memochat::gate::h3::json_support::modules::JsonParseErrorPrefix();
}

std::string ResponseContentType()
{
    return memochat::gate::h3::json_support::modules::ResponseContentType();
}

int ErrorCode()
{
    return memochat::gate::h3::json_support::modules::ErrorCode();
}

int SuccessCode()
{
    return memochat::gate::h3::json_support::modules::SuccessCode();
}

int BadRequestStatus()
{
    return memochat::gate::h3::json_support::modules::BadRequestStatus();
}

int OkStatus()
{
    return memochat::gate::h3::json_support::modules::OkStatus();
}

int InternalErrorStatus()
{
    return memochat::gate::h3::json_support::modules::InternalErrorStatus();
}

bool ShouldForceErrorCode(bool has_error_member, int current_error)
{
    return memochat::gate::h3::json_support::modules::ShouldForceErrorCode(has_error_member, current_error);
}
} // namespace memochat::tests::gate::h3_json_support
