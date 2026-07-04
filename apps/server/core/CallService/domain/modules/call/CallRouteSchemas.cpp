#include "modules/call/CallRouteModule.hpp"

#include "CallPublicDtos.hpp"

import memochat.call.route_schema_algorithms;

namespace memochat::gate::modules::call
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> CallRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;
    namespace modules = memochat::call::route_schema::modules;

    return {
        MakeRouteSchema<memochat::call::CallStartRequestDto, memochat::call::CallStartResponseDto>(
            modules::PostMethod(),
            modules::StartPath(),
            modules::StartRouteName(),
            modules::StartRequestTypeName(),
            modules::StartResponseTypeName()),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            modules::PostMethod(),
            modules::AcceptPath(),
            modules::AcceptRouteName(),
            modules::AuthRequestTypeName(),
            modules::EventResponseTypeName()),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            modules::PostMethod(),
            modules::RejectPath(),
            modules::RejectRouteName(),
            modules::AuthRequestTypeName(),
            modules::EventResponseTypeName()),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            modules::PostMethod(),
            modules::CancelPath(),
            modules::CancelRouteName(),
            modules::AuthRequestTypeName(),
            modules::EventResponseTypeName()),
        MakeRouteSchema<memochat::call::CallAuthRequestDto, memochat::call::CallEventResponseDto>(
            modules::PostMethod(),
            modules::HangupPath(),
            modules::HangupRouteName(),
            modules::AuthRequestTypeName(),
            modules::EventResponseTypeName()),
        MakeRouteSchema<memochat::call::CallTokenRequestDto, memochat::call::CallTokenResponseDto>(
            modules::PostMethod(),
            modules::TokenPath(),
            modules::TokenPostRouteName(),
            modules::TokenRequestTypeName(),
            modules::TokenResponseTypeName()),
    };
}

} // namespace memochat::gate::modules::call
