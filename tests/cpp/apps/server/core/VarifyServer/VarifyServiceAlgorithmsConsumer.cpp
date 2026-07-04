import memochat.varify.service_algorithms;

namespace memochat::tests::varify::service
{
const char* Ipv4PeerPrefix()
{
    return memochat::varify::service::modules::Ipv4PeerPrefix();
}

const char* Ipv6PeerPrefix()
{
    return memochat::varify::service::modules::Ipv6PeerPrefix();
}

int PeerAddressPrefixLength()
{
    return memochat::varify::service::modules::PeerAddressPrefixLength();
}

const char* GrpcLogModuleName()
{
    return memochat::varify::service::modules::GrpcLogModuleName();
}

bool HasGrpcContext(bool context_present)
{
    return memochat::varify::service::modules::HasGrpcContext(context_present);
}

bool HasPeerText(bool peer_empty)
{
    return memochat::varify::service::modules::HasPeerText(peer_empty);
}

bool ShouldSendEmailForSyntheticAccount(bool synthetic_email)
{
    return memochat::varify::service::modules::ShouldSendEmailForSyntheticAccount(synthetic_email);
}
} // namespace memochat::tests::varify::service
