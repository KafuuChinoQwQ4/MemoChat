export module memochat.gate.chat_grpc_client_algorithms;

export namespace memochat::gate::chat_grpc_client::modules
{
int DefaultConnectionPoolSize()
{
    return 5;
}

const char* NotifyCallEventSpanName()
{
    return "ChatService.NotifyCallEvent";
}

const char* ClientSpanKind()
{
    return "CLIENT";
}

const char* RpcSystemAttribute()
{
    return "rpc.system";
}

const char* RpcSystemValue()
{
    return "grpc";
}

const char* RpcServiceAttribute()
{
    return "rpc.service";
}

const char* ChatServiceName()
{
    return "ChatService";
}

const char* RpcMethodAttribute()
{
    return "rpc.method";
}

const char* NotifyGroupEventMethod()
{
    return "NotifyGroupEvent";
}

const char* PeerServiceAttribute()
{
    return "peer_service";
}

bool ShouldWakeConnectionWait(bool stop_requested, bool has_connection)
{
    return stop_requested || has_connection;
}

bool ShouldReturnNullConnection(bool stop_requested)
{
    return stop_requested;
}

bool ShouldAcceptReturnedConnection(bool stop_requested)
{
    return !stop_requested;
}

bool ShouldReportMissingServer(bool pool_found)
{
    return !pool_found;
}

bool ShouldReportUnavailableStub(bool has_stub)
{
    return !has_stub;
}

bool ShouldReportFailedStatus(bool status_ok)
{
    return !status_ok;
}
} // namespace memochat::gate::chat_grpc_client::modules
