import memochat.gate.chat_grpc_client_algorithms;

namespace memochat::tests::gate::chat_grpc_client
{
int DefaultConnectionPoolSize()
{
    return memochat::gate::chat_grpc_client::modules::DefaultConnectionPoolSize();
}

const char* NotifyCallEventSpanName()
{
    return memochat::gate::chat_grpc_client::modules::NotifyCallEventSpanName();
}

const char* ClientSpanKind()
{
    return memochat::gate::chat_grpc_client::modules::ClientSpanKind();
}

const char* RpcSystemAttribute()
{
    return memochat::gate::chat_grpc_client::modules::RpcSystemAttribute();
}

const char* RpcSystemValue()
{
    return memochat::gate::chat_grpc_client::modules::RpcSystemValue();
}

const char* RpcServiceAttribute()
{
    return memochat::gate::chat_grpc_client::modules::RpcServiceAttribute();
}

const char* ChatServiceName()
{
    return memochat::gate::chat_grpc_client::modules::ChatServiceName();
}

const char* RpcMethodAttribute()
{
    return memochat::gate::chat_grpc_client::modules::RpcMethodAttribute();
}

const char* NotifyGroupEventMethod()
{
    return memochat::gate::chat_grpc_client::modules::NotifyGroupEventMethod();
}

const char* PeerServiceAttribute()
{
    return memochat::gate::chat_grpc_client::modules::PeerServiceAttribute();
}

bool ShouldWakeConnectionWait(bool stop_requested, bool has_connection)
{
    return memochat::gate::chat_grpc_client::modules::ShouldWakeConnectionWait(stop_requested, has_connection);
}

bool ShouldReturnNullConnection(bool stop_requested)
{
    return memochat::gate::chat_grpc_client::modules::ShouldReturnNullConnection(stop_requested);
}

bool ShouldAcceptReturnedConnection(bool stop_requested)
{
    return memochat::gate::chat_grpc_client::modules::ShouldAcceptReturnedConnection(stop_requested);
}

bool ShouldReportMissingServer(bool pool_found)
{
    return memochat::gate::chat_grpc_client::modules::ShouldReportMissingServer(pool_found);
}

bool ShouldReportUnavailableStub(bool has_stub)
{
    return memochat::gate::chat_grpc_client::modules::ShouldReportUnavailableStub(has_stub);
}

bool ShouldReportFailedStatus(bool status_ok)
{
    return memochat::gate::chat_grpc_client::modules::ShouldReportFailedStatus(status_ok);
}
} // namespace memochat::tests::gate::chat_grpc_client
