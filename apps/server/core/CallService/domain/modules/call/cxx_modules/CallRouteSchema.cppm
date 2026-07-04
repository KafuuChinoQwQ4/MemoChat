export module memochat.call.route_schema_algorithms;

export namespace memochat::call::route_schema::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* StartPath()
{
    return "/api/call/start";
}

const char* StartRouteName()
{
    return "call.start";
}

const char* StartRequestTypeName()
{
    return "CallStartRequestDto";
}

const char* StartResponseTypeName()
{
    return "CallStartResponseDto";
}

const char* AcceptPath()
{
    return "/api/call/accept";
}

const char* AcceptRouteName()
{
    return "call.accept";
}

const char* RejectPath()
{
    return "/api/call/reject";
}

const char* RejectRouteName()
{
    return "call.reject";
}

const char* CancelPath()
{
    return "/api/call/cancel";
}

const char* CancelRouteName()
{
    return "call.cancel";
}

const char* HangupPath()
{
    return "/api/call/hangup";
}

const char* HangupRouteName()
{
    return "call.hangup";
}

const char* AuthRequestTypeName()
{
    return "CallAuthRequestDto";
}

const char* EventResponseTypeName()
{
    return "CallEventResponseDto";
}

const char* TokenPath()
{
    return "/api/call/token";
}

const char* TokenPostRouteName()
{
    return "call.token.post";
}

const char* TokenRequestTypeName()
{
    return "CallTokenRequestDto";
}

const char* TokenResponseTypeName()
{
    return "CallTokenResponseDto";
}
} // namespace memochat::call::route_schema::modules
