export module memochat.varify.service_algorithms;

export namespace memochat::varify::service::modules
{
const char* Ipv4PeerPrefix()
{
    return "ipv4:";
}

const char* Ipv6PeerPrefix()
{
    return "ipv6:";
}

int PeerAddressPrefixLength()
{
    return 5;
}

const char* GrpcLogModuleName()
{
    return "grpc";
}

bool HasGrpcContext(bool context_present)
{
    return context_present;
}

bool HasPeerText(bool peer_empty)
{
    return !peer_empty;
}

bool ShouldSendEmailForSyntheticAccount(bool synthetic_email)
{
    return !synthetic_email;
}
} // namespace memochat::varify::service::modules
