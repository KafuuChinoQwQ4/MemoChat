#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "runtime/EtcdConfig.h"

namespace memochat::cluster {

struct EtcdServiceEndpoint {
    std::string host;
    std::string tcp_port;
    std::string quic_port;
    std::string rpc_port;
    std::string health_check_url;
    bool healthy = true;
};

struct EtcdServiceRegistration {
    std::string service_name;
    std::string instance_id;
    std::string host;
    uint16_t tcp_port = 0;
    uint16_t quic_port = 0;
    uint16_t rpc_port = 0;
    uint16_t health_port = 0;
    std::string health_check_url;
    int ttl_seconds = 30;

    std::string ToJson() const;
    static std::string EncodeBase64(const std::string& input);
    static std::string DecodeBase64(const std::string& input);
};

class EtcdServiceRegistry {
public:
    using HealthCheckCallback = std::function<bool()>;
    using RegisterCallback = std::function<void(bool success, const std::string& error)>;

    explicit EtcdServiceRegistry(const std::string& endpoints,
                                const std::string& service_name,
                                const std::string& instance_id);
    ~EtcdServiceRegistry();

    bool IsAvailable() const { return _etcd_config && _etcd_config->IsAvailable(); }

    void SetEndpoints(const EtcdServiceEndpoint& endpoints);
    void SetHealthCheck(HealthCheckCallback callback);

    bool Register();
    bool Deregister();
    bool RenewLease();

    void StartHeartbeat();
    void StopHeartbeat();

    void SetRegisterCallback(RegisterCallback callback);

private:
    void HeartbeatLoop();
    std::string BuildServiceKey() const;

    std::shared_ptr<runtime::EtcdConfig> _etcd_config;
    std::string _service_name;
    std::string _instance_id;
    std::string _service_key;

    EtcdServiceEndpoint _endpoints;
    HealthCheckCallback _health_check;
    RegisterCallback _register_callback;

    std::atomic<bool> _registered{false};
    std::atomic<bool> _running{false};
    std::mutex _mutex;
    std::thread _heartbeat_thread;
};

class EtcdClusterDiscovery {
public:
    using NodesChangedCallback = std::function<void(const std::vector<ChatNodeDescriptor>& nodes)>;

    explicit EtcdClusterDiscovery(const std::string& endpoints,
                                  const std::string& service_namespace,
                                  const std::string& self_node_name = "");
    ~EtcdClusterDiscovery();

    bool IsAvailable() const { return _etcd_config && _etcd_config->IsAvailable(); }

    ChatClusterConfig DiscoverCluster();
    std::vector<ChatNodeDescriptor> GetNodes();
    std::optional<ChatNodeDescriptor> GetSelfNode();

    void StartWatching();
    void StopWatching();

    void SetNodesChangedCallback(NodesChangedCallback callback);

    static std::string BuildNodeKey(const std::string& namespace_name, const std::string& node_name);
    static ChatNodeDescriptor ParseNodeValue(const std::string& json);

private:
    void OnNodeChange(const std::string& key, const std::string& value);
    void LoadNodesFromEtcd();
    std::string BuildNodesPrefix() const;

    std::shared_ptr<runtime::EtcdConfig> _etcd_config;
    std::string _namespace;
    std::string _self_node_name;
    std::string _nodes_prefix;

    std::mutex _nodes_mutex;
    std::vector<ChatNodeDescriptor> _nodes;

    NodesChangedCallback _nodes_changed_callback;
    std::atomic<bool> _watching{false};
};

class EtcdClusterDiscoveryLoader {
public:
    static std::unique_ptr<EtcdClusterDiscovery> TryCreate(const ConfigValueGetter& getter,
                                                            const std::string& self_node_name = "");
};

} // namespace memochat::cluster
