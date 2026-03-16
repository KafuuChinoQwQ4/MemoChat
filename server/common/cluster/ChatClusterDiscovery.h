#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace memochat::cluster {

struct ChatNodeDescriptor {
    std::string name;
    std::string tcp_host;
    std::string tcp_port;
    std::string quic_host;
    std::string quic_port;
    std::string rpc_host;
    std::string rpc_port;
    bool enabled = true;
};

struct ChatClusterConfig {
    std::string discovery_mode = "static";
    std::vector<ChatNodeDescriptor> nodes;

    const ChatNodeDescriptor* findNode(const std::string& name) const {
        for (const auto& node : nodes) {
            if (node.name == name) {
                return &node;
            }
        }
        return nullptr;
    }

    std::vector<ChatNodeDescriptor> enabledNodes() const {
        std::vector<ChatNodeDescriptor> enabled;
        for (const auto& node : nodes) {
            if (node.enabled) {
                enabled.push_back(node);
            }
        }
        return enabled;
    }
};

using ConfigValueGetter = std::function<std::string(const std::string&, const std::string&)>;

inline std::string TrimCopy(std::string text) {
    const auto not_space = [](unsigned char ch) {
        return !std::isspace(ch);
    };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
    return text;
}

inline std::vector<std::string> SplitCsv(const std::string& csv) {
    std::vector<std::string> values;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = TrimCopy(item);
        if (!item.empty()) {
            values.push_back(item);
        }
    }
    return values;
}

inline bool ParseEnabledFlag(const std::string& raw) {
    const std::string value = TrimCopy(raw);
    if (value.empty()) {
        return true;
    }

    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

inline std::string ReadRequiredValue(const ConfigValueGetter& getter,
                                     const std::string& section,
                                     const std::string& key) {
    const std::string value = TrimCopy(getter(section, key));
    if (value.empty()) {
        throw std::runtime_error("missing required cluster config: [" + section + "] " + key);
    }
    return value;
}

inline std::string ReadOptionalValue(const ConfigValueGetter& getter,
                                     const std::string& section,
                                     const std::string& key) {
    return getter ? TrimCopy(getter(section, key)) : "";
}

inline ChatClusterConfig LoadStaticChatClusterConfig(const ConfigValueGetter& getter,
                                                     const std::string& self_node_name = "") {
    if (!getter) {
        throw std::runtime_error("cluster config getter is not set");
    }

    ChatClusterConfig config;
    const std::string discovery_mode = TrimCopy(getter("Cluster", "DiscoveryMode"));
    if (!discovery_mode.empty()) {
        config.discovery_mode = discovery_mode;
    }
    if (config.discovery_mode != "static") {
        throw std::runtime_error("unsupported cluster discovery mode: " + config.discovery_mode);
    }

    const std::vector<std::string> node_sections = SplitCsv(ReadRequiredValue(getter, "Cluster", "Nodes"));
    if (node_sections.empty()) {
        throw std::runtime_error("cluster config has no node sections");
    }

    std::unordered_set<std::string> seen_names;
    std::set<std::pair<std::string, std::string>> tcp_endpoints;
    std::set<std::pair<std::string, std::string>> quic_endpoints;
    std::set<std::pair<std::string, std::string>> rpc_endpoints;

    for (const auto& section : node_sections) {
        ChatNodeDescriptor node;
        node.name = ReadRequiredValue(getter, section, "Name");
        node.tcp_host = ReadRequiredValue(getter, section, "TcpHost");
        node.tcp_port = ReadRequiredValue(getter, section, "TcpPort");
        node.quic_host = ReadOptionalValue(getter, section, "QuicHost");
        node.quic_port = ReadOptionalValue(getter, section, "QuicPort");
        node.rpc_host = ReadRequiredValue(getter, section, "RpcHost");
        node.rpc_port = ReadRequiredValue(getter, section, "RpcPort");
        node.enabled = ParseEnabledFlag(getter(section, "Enabled"));

        if (!seen_names.insert(node.name).second) {
            throw std::runtime_error("duplicate chat node name: " + node.name);
        }

        const auto tcp_endpoint = std::make_pair(node.tcp_host, node.tcp_port);
        if (!tcp_endpoints.insert(tcp_endpoint).second) {
            throw std::runtime_error("duplicate chat tcp endpoint: " + node.tcp_host + ":" + node.tcp_port);
        }

        if (!node.quic_host.empty() && !node.quic_port.empty()) {
            const auto quic_endpoint = std::make_pair(node.quic_host, node.quic_port);
            if (!quic_endpoints.insert(quic_endpoint).second) {
                throw std::runtime_error("duplicate chat quic endpoint: " + node.quic_host + ":" + node.quic_port);
            }
        }

        const auto rpc_endpoint = std::make_pair(node.rpc_host, node.rpc_port);
        if (!rpc_endpoints.insert(rpc_endpoint).second) {
            throw std::runtime_error("duplicate chat rpc endpoint: " + node.rpc_host + ":" + node.rpc_port);
        }

        config.nodes.push_back(node);
    }

    if (config.enabledNodes().empty()) {
        throw std::runtime_error("cluster config has no enabled chat nodes");
    }

    if (!self_node_name.empty()) {
        const ChatNodeDescriptor* self_node = config.findNode(self_node_name);
        if (self_node == nullptr) {
            throw std::runtime_error("self chat node missing from cluster config: " + self_node_name);
        }
        if (!self_node->enabled) {
            throw std::runtime_error("self chat node is disabled in cluster config: " + self_node_name);
        }
    }

    return config;
}

inline std::string GetEnvOrEmpty(const char* name) {
    if (name == nullptr || *name == '\0') {
        return "";
    }
    const char* value = std::getenv(name);
    return value == nullptr ? "" : std::string(value);
}

inline int ParsePositiveIntOrDefault(const std::string& raw, int fallback) {
    if (raw.empty()) {
        return fallback;
    }
    try {
        const int value = std::stoi(raw);
        return value > 0 ? value : fallback;
    } catch (...) {
        return fallback;
    }
}

inline std::string ResolveSelfNodeName(const ConfigValueGetter& getter) {
    std::string self_name = TrimCopy(GetEnvOrEmpty("MEMOCHAT_INSTANCE_NAME"));
    if (!self_name.empty()) {
        return self_name;
    }

    self_name = TrimCopy(GetEnvOrEmpty("MEMOCHAT_POD_NAME"));
    if (!self_name.empty()) {
        return self_name;
    }

    self_name = TrimCopy(GetEnvOrEmpty("HOSTNAME"));
    if (!self_name.empty()) {
        return self_name;
    }

    if (getter) {
        self_name = TrimCopy(getter("SelfServer", "Name"));
    }
    return self_name;
}

inline ChatClusterConfig LoadK8sStatefulSetChatClusterConfig(const ConfigValueGetter& getter,
                                                             const std::string& self_node_name = "") {
    if (!getter) {
        throw std::runtime_error("cluster config getter is not set");
    }

    ChatClusterConfig config;
    config.discovery_mode = "k8s-statefulset";

    const std::string service_name = ReadRequiredValue(getter, "Cluster", "ServiceName");
    const std::string namespace_name = ReadRequiredValue(getter, "Cluster", "Namespace");
    const int replicas = ParsePositiveIntOrDefault(ReadRequiredValue(getter, "Cluster", "Replicas"), 1);
    const std::string tcp_port = ReadRequiredValue(getter, "Cluster", "TcpPort");
    const std::string quic_port = ReadOptionalValue(getter, "Cluster", "QuicPort");
    const std::string rpc_port = ReadRequiredValue(getter, "Cluster", "RpcPort");
    const std::string domain = TrimCopy(getter("Cluster", "Domain"));
    const std::string cluster_domain = domain.empty() ? "cluster.local" : domain;

    std::unordered_set<std::string> seen_names;
    for (int ordinal = 0; ordinal < replicas; ++ordinal) {
        ChatNodeDescriptor node;
        node.name = service_name + "-" + std::to_string(ordinal);
        node.tcp_host = node.name + "." + service_name + "." + namespace_name + ".svc." + cluster_domain;
        node.tcp_port = tcp_port;
        node.quic_host = quic_port.empty() ? "" : node.tcp_host;
        node.quic_port = quic_port;
        node.rpc_host = node.tcp_host;
        node.rpc_port = rpc_port;
        node.enabled = true;
        if (!seen_names.insert(node.name).second) {
            throw std::runtime_error("duplicate k8s chat node name: " + node.name);
        }
        config.nodes.push_back(node);
    }

    if (config.nodes.empty()) {
        throw std::runtime_error("k8s cluster config has no nodes");
    }

    const std::string resolved_self = self_node_name.empty() ? ResolveSelfNodeName(getter) : self_node_name;
    if (!resolved_self.empty() && config.findNode(resolved_self) == nullptr) {
        throw std::runtime_error("self chat node missing from k8s cluster config: " + resolved_self);
    }

    return config;
}

inline ChatClusterConfig LoadChatClusterConfig(const ConfigValueGetter& getter,
                                               const std::string& self_node_name = "") {
    if (!getter) {
        throw std::runtime_error("cluster config getter is not set");
    }

    const std::string discovery_mode = TrimCopy(getter("Cluster", "DiscoveryMode"));
    if (discovery_mode == "k8s-statefulset") {
        return LoadK8sStatefulSetChatClusterConfig(getter, self_node_name);
    }
    return LoadStaticChatClusterConfig(getter, self_node_name);
}

} // namespace memochat::cluster
