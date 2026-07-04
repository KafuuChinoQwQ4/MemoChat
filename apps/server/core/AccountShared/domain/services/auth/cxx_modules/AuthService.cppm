export module memochat.account.auth_service_algorithms;

export namespace memochat::account::auth_service::modules
{
int ResponseOkStatus()
{
    return 200;
}

const char* JsonContentType()
{
    return "application/json";
}

bool FeatureGroupChatEnabled()
{
    return true;
}

// RegisterUser returns 0 or -1 to signal "user or email exists"; any other
// value is a valid newly-registered uid.
bool IsRegisteredUidValid(int uid)
{
    return uid != 0 && uid != -1;
}

const char* FallbackTransport()
{
    return "tcp";
}

const char* QuicTransport()
{
    return "quic";
}

const char* TcpTransport()
{
    return "tcp";
}

// A chat route node prefers QUIC only when it advertises both a QUIC host and
// a QUIC port; otherwise the login response falls back to TCP.
const char* PreferredTransport(bool has_quic_host, bool has_quic_port)
{
    return (has_quic_host && has_quic_port) ? QuicTransport() : TcpTransport();
}
} // namespace memochat::account::auth_service::modules
