import memochat.varify.email_sender_algorithms;

namespace memochat::tests::varify::email_sender
{
int DefaultSmtpPort()
{
    return memochat::varify::email_sender::modules::DefaultSmtpPort();
}

bool ShouldUseImplicitSsl(int port)
{
    return memochat::varify::email_sender::modules::ShouldUseImplicitSsl(port);
}

bool HasStatusCodePrefix(unsigned long long line_size)
{
    return memochat::varify::email_sender::modules::HasStatusCodePrefix(line_size);
}

bool IsMultilineReply(unsigned long long line_size, char separator)
{
    return memochat::varify::email_sender::modules::IsMultilineReply(line_size, separator);
}

bool IsExpectedStatusCode(int actual_code, int expected_code)
{
    return memochat::varify::email_sender::modules::IsExpectedStatusCode(actual_code, expected_code);
}
} // namespace memochat::tests::varify::email_sender
