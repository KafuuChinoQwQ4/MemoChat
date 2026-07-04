export module memochat.call.route_module_algorithms;

export namespace memochat::call::route_modules
{
const char* GetMethod()
{
    return "GET";
}

const char* PostMethod()
{
    return "POST";
}

const char* StartPath()
{
    return "/api/call/start";
}

const char* AcceptPath()
{
    return "/api/call/accept";
}

const char* RejectPath()
{
    return "/api/call/reject";
}

const char* CancelPath()
{
    return "/api/call/cancel";
}

const char* HangupPath()
{
    return "/api/call/hangup";
}

const char* TokenPath()
{
    return "/api/call/token";
}

int ParseUnsignedDecimalOrZero(const char* data, unsigned long long size)
{
    int value = 0;
    for (unsigned long long index = 0; index < size; ++index)
    {
        const auto ch = static_cast<unsigned char>(data[index]);
        if (ch < static_cast<unsigned char>('0') || ch > static_cast<unsigned char>('9'))
        {
            return index == 0ULL ? 0 : value;
        }
        const int digit = static_cast<int>(ch - static_cast<unsigned char>('0'));
        if (value > (2147483647 - digit) / 10)
        {
            return 0;
        }
        value = value * 10 + digit;
    }
    return value;
}

bool IsJsonObjectTailForTraceAppend(bool body_empty, char last_char, bool already_has_trace_id)
{
    return !already_has_trace_id && !body_empty && last_char == '}';
}
} // namespace memochat::call::route_modules
