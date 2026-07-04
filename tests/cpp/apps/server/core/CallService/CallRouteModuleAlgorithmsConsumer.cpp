import memochat.call.route_module_algorithms;

namespace memochat::tests::call
{
const char* GetMethod()
{
    return memochat::call::route_modules::GetMethod();
}

const char* PostMethod()
{
    return memochat::call::route_modules::PostMethod();
}

const char* StartPath()
{
    return memochat::call::route_modules::StartPath();
}

const char* AcceptPath()
{
    return memochat::call::route_modules::AcceptPath();
}

const char* RejectPath()
{
    return memochat::call::route_modules::RejectPath();
}

const char* CancelPath()
{
    return memochat::call::route_modules::CancelPath();
}

const char* HangupPath()
{
    return memochat::call::route_modules::HangupPath();
}

const char* TokenPath()
{
    return memochat::call::route_modules::TokenPath();
}

int ParseRouteUid(const char* data, unsigned long long size)
{
    return memochat::call::route_modules::ParseUnsignedDecimalOrZero(data, size);
}

bool ShouldAppendTraceId(bool body_empty, char last_char, bool already_has_trace_id)
{
    return memochat::call::route_modules::IsJsonObjectTailForTraceAppend(body_empty, last_char, already_has_trace_id);
}
} // namespace memochat::tests::call
