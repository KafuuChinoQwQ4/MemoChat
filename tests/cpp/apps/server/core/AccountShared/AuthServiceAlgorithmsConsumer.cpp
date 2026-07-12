#include <string_view>

import memochat.account.auth_service_algorithms;

namespace memochat::tests::account::auth_service
{
int ResponseOkStatus()
{
    return memochat::account::auth_service::modules::ResponseOkStatus();
}

const char* JsonContentType()
{
    return memochat::account::auth_service::modules::JsonContentType();
}

bool FeatureGroupChatEnabled()
{
    return memochat::account::auth_service::modules::FeatureGroupChatEnabled();
}

bool IsRegisteredUidValid(int uid)
{
    return memochat::account::auth_service::modules::IsRegisteredUidValid(uid);
}

const char* FallbackTransport()
{
    return memochat::account::auth_service::modules::FallbackTransport();
}

const char* QuicTransport()
{
    return memochat::account::auth_service::modules::QuicTransport();
}

const char* WebSocketTransport()
{
    return memochat::account::auth_service::modules::WebSocketTransport();
}

const char* WebTransportTransport()
{
    return memochat::account::auth_service::modules::WebTransportTransport();
}

const char* TcpTransport()
{
    return memochat::account::auth_service::modules::TcpTransport();
}

const char* PreferredTransport(bool has_quic_host, bool has_quic_port)
{
    return memochat::account::auth_service::modules::PreferredTransport(has_quic_host, has_quic_port);
}

bool ShouldIssueRefreshToken(std::string_view client_marker)
{
    return memochat::account::auth_service::modules::ShouldIssueRefreshToken(client_marker.data(),
                                                                             client_marker.size());
}
} // namespace memochat::tests::account::auth_service
