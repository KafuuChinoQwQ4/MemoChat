export module memochat.varify.email_sender_algorithms;

export namespace memochat::varify::email_sender::modules
{
int DefaultSmtpPort()
{
    return 465;
}

bool ShouldUseImplicitSsl(int port)
{
    return port == DefaultSmtpPort();
}

bool HasStatusCodePrefix(unsigned long long line_size)
{
    return line_size >= 3;
}

bool HasMultilineSeparatorPosition(unsigned long long line_size)
{
    return line_size > 3;
}

bool IsMultilineReplySeparator(char separator)
{
    return separator == '-';
}

bool IsMultilineReply(unsigned long long line_size, char separator)
{
    return HasMultilineSeparatorPosition(line_size) && IsMultilineReplySeparator(separator);
}

bool IsExpectedStatusCode(int actual_code, int expected_code)
{
    return actual_code == expected_code;
}
} // namespace memochat::varify::email_sender::modules
