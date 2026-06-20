#include "modules/call/CallRouteModule.h"

#include "CallPublicDtos.h"

namespace memochat::gate::modules::call
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> CallRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<memochat::call::CallStartRequestDto, memochat::call::CallStartResponseDto>(
            "POST",
            "/api/call/start",
            "call.start",
            "CallStartRequestDto",
            "CallStartResponseDto"),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            "POST",
            "/api/call/accept",
            "call.accept",
            "CallAuthRequestDto",
            "CallEventResponseDto"),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            "POST",
            "/api/call/reject",
            "call.reject",
            "CallAuthRequestDto",
            "CallEventResponseDto"),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            "POST",
            "/api/call/cancel",
            "call.cancel",
            "CallAuthRequestDto",
            "CallEventResponseDto"),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            "POST",
            "/api/call/hangup",
            "call.hangup",
            "CallAuthRequestDto",
            "CallEventResponseDto"),
        MakeRouteSchema<memochat::call::CallTokenRequestDto, memochat::call::CallTokenResponseDto>(
            "POST",
            "/api/call/token",
            "call.token.post",
            "CallTokenRequestDto",
            "CallTokenResponseDto"),
    };
}

} // namespace memochat::gate::modules::call
