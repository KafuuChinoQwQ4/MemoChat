import memochat.chat.chat_grpc_client_algorithms;

#include <string>

namespace modules = memochat::chat::chat_grpc_client::modules;

bool MemoChatTestChatGrpcShouldSkipSelfNode(const std::string& node_name, const std::string& self_name)
{
    return modules::ShouldSkipSelfNode(node_name.data(), node_name.size(), self_name.data(), self_name.size());
}

int MemoChatTestChatGrpcDefaultRemotePoolSize()
{
    return modules::DefaultRemotePoolSize();
}

bool MemoChatTestChatGrpcShouldSkipRemoteCall(bool pool_found)
{
    return modules::ShouldSkipRemoteCall(pool_found);
}

bool MemoChatTestChatGrpcShouldReportStatusError(bool status_ok)
{
    return modules::ShouldReportGrpcStatusError(status_ok);
}
