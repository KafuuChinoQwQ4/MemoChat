#include "cluster/EtcdClusterDiscovery.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace memochat::cluster {

std::string EtcdServiceRegistration::ToJson() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"serviceName\":\"" << service_name << "\",";
    oss << "\"instanceId\":\"" << instance_id << "\",";
    oss << "\"host\":\"" << host << "\",";
    oss << "\"tcpPort\":" << tcp_port << ",";
    oss << "\"quicPort\":" << quic_port << ",";
    oss << "\"rpcPort\":" << rpc_port << ",";
    oss << "\"healthPort\":" << health_port << ",";
    oss << "\"healthCheckUrl\":\"" << health_check_url << "\",";
    oss << "\"ttlSeconds\":" << ttl_seconds;
    oss << "}";
    return oss.str();
}

std::string EtcdServiceRegistration::EncodeBase64(const std::string& input) {
    static const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = static_cast<int>(input.size());
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(input.c_str());

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++) {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++) {
            ret += base64_chars[char_array_4[j]];
        }
        while((i++ < 3)) {
            ret += '=';
        }
    }
    return ret;
}

std::string EtcdServiceRegistration::DecodeBase64(const std::string& input) {
    static const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    int i = 0;
    int j = 0;
    int in_len = static_cast<int>(input.size());
    const unsigned char* bytes_to_decode = reinterpret_cast<const unsigned char*>(input.c_str());
    unsigned char char_array_4[4];

    while (in_len-- && (bytes_to_decode[i] != '=') && IsBase64(bytes_to_decode[i])) {
        char_array_4[j++] = bytes_to_decode[i++];
        if (j == 4) {
            char_array_4[0] = (((char_array_4[0] & 0x3f) << 2) + ((char_array_4[1] & 0x30) >> 4));
            char_array_4[1] = (((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
            char_array_4[2] = (((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

            for(i = 0; i < 3; i++) {
                ret += static_cast<char>(char_array_4[i]);
            }
            j = 0;
        }
    }
    if (j)    {
        for(int k = 0; k < j; k++) {
            ret += base64_chars[char_array_4[k]];
        }
    }
    return ret;
}

bool EtcdServiceRegistration::IsBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

EtcdServiceRegistry::EtcdServiceRegistry(const std::string& endpoints,
                                       const std::string& service_name,
                                       const std::string& instance_id)
    : _service_name(service_name)
    , _instance_id(instance_id) {
    _etcd_config = runtime::EtcdConfigLoader::TryCreate(endpoints, "registry");
}

EtcdServiceRegistry::~EtcdServiceRegistry() {
    StopHeartbeat();
    Deregister();
}

void EtcdServiceRegistry::SetEndpoints(const EtcdServiceEndpoint& endpoints) {
    std::lock_guard<std::mutex> lock(_mutex);
    _endpoints = endpoints;
}

void EtcdServiceRegistry::SetHealthCheck(HealthCheckCallback callback) {
    _health_check = callback;
}

std::string EtcdServiceRegistry::BuildServiceKey() const {
    return "/memochat/nodes/" + _service_name + "/" + _instance_id;
}

bool EtcdServiceRegistry::Register() {
    if (!_etcd_config || !_etcd_config->IsAvailable()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    EtcdServiceRegistration reg;
    reg.service_name = _service_name;
    reg.instance_id = _instance_id;
    reg.host = _endpoints.host;
    reg.tcp_port = static_cast<uint16_t>(std::stoi(_endpoints.tcp_port));
    reg.quic_port = static_cast<uint16_t>(std::stoi(_endpoints.quic_port));
    reg.rpc_port = static_cast<uint16_t>(std::stoi(_endpoints.rpc_port));
    reg.health_port = static_cast<uint16_t>(std::stoi(_endpoints.health_check_url));
    reg.health_check_url = _endpoints.health_check_url;
    reg.ttl_seconds = 30;

    const std::string key = BuildServiceKey();
    const std::string value = reg.ToJson();
    const std::string encoded_value = EtcdServiceRegistration::EncodeBase64(value);

    bool success = _etcd_config->Get("registry", key, const_cast<std::string&>(encoded_value));
    _registered.store(success);

    if (_register_callback) {
        _register_callback(success, success ? "" : "Failed to register service");
    }

    return success;
}

bool EtcdServiceRegistry::Deregister() {
    if (!_etcd_config || !_registered.load()) {
        return true;
    }

    const std::string key = BuildServiceKey();
    bool success = _etcd_config->Get("registry", key, const_cast<std::string&>(""));
    _registered.store(false);

    return success;
}

bool EtcdServiceRegistry::RenewLease() {
    return Register();
}

void EtcdServiceRegistry::StartHeartbeat() {
    if (_running.load()) {
        return;
    }

    _running.store(true);
    _heartbeat_thread = std::thread(&EtcdServiceRegistry::HeartbeatLoop, this);
}

void EtcdServiceRegistry::StopHeartbeat() {
    _running.store(false);
    if (_heartbeat_thread.joinable()) {
        _heartbeat_thread.join();
    }
}

void EtcdServiceRegistry::SetRegisterCallback(RegisterCallback callback) {
    _register_callback = callback;
}

void EtcdServiceRegistry::HeartbeatLoop() {
    while (_running.load()) {
        if (_health_check && !_health_check()) {
            std::cerr << "[EtcdServiceRegistry] Health check failed, stopping heartbeat" << std::endl;
            break;
        }

        if (!RenewLease()) {
            std::cerr << "[EtcdServiceRegistry] Failed to renew lease" << std::endl;
        }

        for (int i = 0; i < 15 && _running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

std::string EtcdClusterDiscovery::BuildNodeKey(const std::string& namespace_name, const std::string& node_name) {
    return "/memochat/nodes/" + namespace_name + "/" + node_name;
}

ChatNodeDescriptor EtcdClusterDiscovery::ParseNodeValue(const std::string& json) {
    ChatNodeDescriptor node;

    try {
        std::istringstream iss(json);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(iss, pt);

        node.name = pt.get<std::string>("instanceId", "");
        node.tcp_host = pt.get<std::string>("host", "");
        node.tcp_port = std::to_string(pt.get<int>("tcpPort", 0));
        node.quic_host = pt.get<std::string>("host", "");
        node.quic_port = std::to_string(pt.get<int>("quicPort", 0));
        node.rpc_host = pt.get<std::string>("host", "");
        node.rpc_port = std::to_string(pt.get<int>("rpcPort", 0));
        node.enabled = pt.get<bool>("healthy", true);
    } catch (const std::exception& e) {
        std::cerr << "[EtcdClusterDiscovery] Failed to parse node value: " << e.what() << std::endl;
    }

    return node;
}

EtcdClusterDiscovery::EtcdClusterDiscovery(const std::string& endpoints,
                                          const std::string& service_namespace,
                                          const std::string& self_node_name)
    : _namespace(service_namespace)
    , _self_node_name(self_node_name)
    , _nodes_prefix("/memochat/nodes/" + service_namespace + "/") {
    _etcd_config = runtime::EtcdConfigLoader::TryCreate(endpoints, "discovery");
}

EtcdClusterDiscovery::~EtcdClusterDiscovery() {
    StopWatching();
}

ChatClusterConfig EtcdClusterDiscovery::DiscoverCluster() {
    ChatClusterConfig config;
    config.discovery_mode = "etcd";

    LoadNodesFromEtcd();

    std::lock_guard<std::mutex> lock(_nodes_mutex);
    config.nodes = _nodes;

    return config;
}

std::vector<ChatNodeDescriptor> EtcdClusterDiscovery::GetNodes() {
    std::lock_guard<std::mutex> lock(_nodes_mutex);
    return _nodes;
}

std::optional<ChatNodeDescriptor> EtcdClusterDiscovery::GetSelfNode() {
    if (_self_node_name.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(_nodes_mutex);
    for (const auto& node : _nodes) {
        if (node.name == _self_node_name) {
            return node;
        }
    }
    return std::nullopt;
}

void EtcdClusterDiscovery::StartWatching() {
    if (_watching.load()) {
        return;
    }

    _watching.store(true);

    if (_etcd_config) {
        _etcd_config->SetChangeCallback([this](const std::string& section, const std::string& key, const std::string& value) {
            OnNodeChange(key, value);
        });
        _etcd_config->StartWatch();
    }
}

void EtcdClusterDiscovery::StopWatching() {
    _watching.store(false);
    if (_etcd_config) {
        _etcd_config->Stop();
    }
}

void EtcdClusterDiscovery::SetNodesChangedCallback(NodesChangedCallback callback) {
    _nodes_changed_callback = callback;
}

void EtcdClusterDiscovery::OnNodeChange(const std::string& key, const std::string& value) {
    if (key.find(_nodes_prefix) != 0) {
        return;
    }

    std::string node_name = key.substr(_nodes_prefix.size());
    size_t slash_pos = node_name.find('/');
    if (slash_pos != std::string::npos) {
        node_name = node_name.substr(0, slash_pos);
    }

    if (value.empty()) {
        std::lock_guard<std::mutex> lock(_nodes_mutex);
        _nodes.erase(
            std::remove_if(_nodes.begin(), _nodes.end(),
                [&node_name](const ChatNodeDescriptor& node) { return node.name == node_name; }),
            _nodes.end()
        );
    } else {
        ChatNodeDescriptor node = ParseNodeValue(value);
        if (!node.name.empty()) {
            std::lock_guard<std::mutex> lock(_nodes_mutex);
            bool found = false;
            for (auto& existing : _nodes) {
                if (existing.name == node.name) {
                    existing = node;
                    found = true;
                    break;
                }
            }
            if (!found) {
                _nodes.push_back(node);
            }
        }
    }

    if (_nodes_changed_callback) {
        std::lock_guard<std::mutex> lock(_nodes_mutex);
        _nodes_changed_callback(_nodes);
    }
}

void EtcdClusterDiscovery::LoadNodesFromEtcd() {
    if (!_etcd_config || !_etcd_config->IsAvailable()) {
        std::cerr << "[EtcdClusterDiscovery] etcd not available" << std::endl;
        return;
    }

    std::vector<std::pair<std::string, std::string>> all_nodes;
    _etcd_config->Get("discovery", _nodes_prefix, const_cast<std::string&>(""));

    std::lock_guard<std::mutex> lock(_nodes_mutex);
    _nodes.clear();

    for (const auto& [key, value] : all_nodes) {
        ChatNodeDescriptor node = ParseNodeValue(value);
        if (!node.name.empty()) {
            _nodes.push_back(node);
        }
    }
}

std::string EtcdClusterDiscovery::BuildNodesPrefix() const {
    return _nodes_prefix;
}

std::unique_ptr<EtcdClusterDiscovery> EtcdClusterDiscoveryLoader::TryCreate(const ConfigValueGetter& getter,
                                                                            const std::string& self_node_name) {
    if (!getter) {
        return nullptr;
    }

    const std::string discovery_mode = TrimCopy(getter("Cluster", "DiscoveryMode"));
    if (discovery_mode != "etcd") {
        return nullptr;
    }

    const std::string endpoints = TrimCopy(getter("Cluster", "EtcdEndpoints"));
    if (endpoints.empty()) {
        std::cerr << "[EtcdClusterDiscoveryLoader] etcd endpoints not configured" << std::endl;
        return nullptr;
    }

    const std::string service_namespace = TrimCopy(getter("Cluster", "ServiceNamespace"));
    if (service_namespace.empty()) {
        std::cerr << "[EtcdClusterDiscoveryLoader] service namespace not configured" << std::endl;
        return nullptr;
    }

    auto discovery = std::make_unique<EtcdClusterDiscovery>(endpoints, service_namespace, self_node_name);

    if (!discovery->IsAvailable()) {
        std::cerr << "[EtcdClusterDiscoveryLoader] Failed to connect to etcd" << std::endl;
        return nullptr;
    }

    std::cout << "[EtcdClusterDiscoveryLoader] Successfully created etcd discovery" << std::endl;
    return discovery;
}

} // namespace memochat::cluster
