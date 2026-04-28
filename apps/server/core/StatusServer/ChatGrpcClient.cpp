#include "ChatGrpcClient.h"
#include "RedisMgr.h"
#include "cluster/ChatClusterDiscovery.h"

ChatGrpcClient::ChatGrpcClient()
{
	auto& cfg = ConfigMgr::Inst();
	auto cluster = memochat::cluster::LoadChatClusterConfig(
		[&cfg](const std::string& section, const std::string& key) {
			return cfg.GetValue(section, key);
		});

	for (const auto& node : cluster.enabledNodes()) {
		_pools[node.name] = std::make_unique<ChatConPool>(5, node.rpc_host, node.rpc_port);
	}
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(const AddFriendReq& req)
{
	auto to_uid = req.touid();
	std::string  uid_str = std::to_string(to_uid);
	
	AddFriendRsp rsp;
	return rsp;
}
