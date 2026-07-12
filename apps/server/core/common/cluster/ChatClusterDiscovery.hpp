#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace memochat::cluster
{

struct ChatNodeDescriptor
{
    std::string name;
    std::string tcp_host;
    std::string tcp_port;
    std::string quic_host;
    std::string quic_port;
    bool ws_enabled = false;
    std::string ws_host;
    std::string ws_port;
    std::string ws_path;
    bool ws_tls = false;
    bool wt_enabled = false;
    std::string wt_host;
    std::string wt_port;
    std::string wt_path;
    bool wt_tls = false;
    std::string rpc_host;
    std::string rpc_port;
    bool enabled = true;
};

struct ChatClusterConfig
{
    std::string discovery_mode = "static";
    std::vector<ChatNodeDescriptor> nodes;

    const ChatNodeDescriptor* findNode(const std::string& name) const
    {
        for (const auto& node : nodes)
        {
            if (node.name == name)
            {
                return &node;
            }
        }
        return nullptr;
    }

    std::vector<ChatNodeDescriptor> enabledNodes() const
    {
        std::vector<ChatNodeDescriptor> enabled;
        for (const auto& node : nodes)
        {
            if (node.enabled)
            {
                enabled.push_back(node);
            }
        }
        return enabled;
    }
};

using ConfigValueGetter = std::function<std::string(const std::string&, const std::string&)>;

inline std::string TrimCopy(std::string text)
{
    const auto not_space = [](unsigned char ch)
    {
        return !std::isspace(ch);
    };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
    return text;
}

inline std::vector<std::string> SplitCsv(const std::string& csv)
{
    std::vector<std::string> values;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        item = TrimCopy(item);
        if (!item.empty())
        {
            values.push_back(item);
        }
    }
    return values;
}

inline bool ParseEnabledFlag(const std::string& raw)
{
    const std::string value = TrimCopy(raw);
    if (value.empty())
    {
        return true;
    }

    std::string lower = value;
    std::transform(lower.begin(),
                   lower.end(),
                   lower.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

inline bool ParseOptionalFlag(const std::string& raw, bool fallback)
{
    const std::string value = TrimCopy(raw);
    if (value.empty())
    {
        return fallback;
    }

    std::string lower = value;
    std::transform(lower.begin(),
                   lower.end(),
                   lower.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on")
    {
        return true;
    }
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off")
    {
        return false;
    }
    return fallback;
}

inline bool ReadRequiredValue(const ConfigValueGetter& getter,
                              const std::string& section,
                              const std::string& key,
                              std::string& value,
                              std::string& error)
{
    value = TrimCopy(getter(section, key));
    if (value.empty())
    {
        error = "missing required cluster config: [" + section + "] " + key;
        return false;
    }
    return true;
}

inline std::string
ReadOptionalValue(const ConfigValueGetter& getter, const std::string& section, const std::string& key)
{
    return getter ? TrimCopy(getter(section, key)) : "";
}

inline bool LoadStaticChatClusterConfig(const ConfigValueGetter& getter,
                                        const std::string& self_node_name,
                                        ChatClusterConfig& out,
                                        std::string& error)
{
    error.clear();
    if (!getter)
    {
        error = "cluster config getter is not set";
        return false;
    }

    ChatClusterConfig config;
    const std::string discovery_mode = TrimCopy(getter("Cluster", "DiscoveryMode"));
    if (!discovery_mode.empty())
    {
        config.discovery_mode = discovery_mode;
    }
    if (config.discovery_mode != "static")
    {
        error = "unsupported cluster discovery mode: " + config.discovery_mode;
        return false;
    }

    std::string nodes_value;
    if (!ReadRequiredValue(getter, "Cluster", "Nodes", nodes_value, error))
    {
        return false;
    }
    const std::vector<std::string> node_sections = SplitCsv(nodes_value);
    if (node_sections.empty())
    {
        error = "cluster config has no node sections";
        return false;
    }

    std::unordered_set<std::string> seen_names;
    std::set<std::pair<std::string, std::string>> tcp_endpoints;
    std::set<std::pair<std::string, std::string>> quic_endpoints;
    std::set<std::pair<std::string, std::string>> ws_endpoints;
    std::set<std::pair<std::string, std::string>> wt_endpoints;
    std::set<std::pair<std::string, std::string>> rpc_endpoints;

    for (const auto& section : node_sections)
    {
        ChatNodeDescriptor node;
        if (!ReadRequiredValue(getter, section, "Name", node.name, error) ||
            !ReadRequiredValue(getter, section, "TcpHost", node.tcp_host, error) ||
            !ReadRequiredValue(getter, section, "TcpPort", node.tcp_port, error))
        {
            return false;
        }
        node.quic_host = ReadOptionalValue(getter, section, "QuicHost");
        node.quic_port = ReadOptionalValue(getter, section, "QuicPort");
        node.ws_enabled = ParseOptionalFlag(getter(section, "WsEnabled"), false);
        node.ws_host = ReadOptionalValue(getter, section, "WsHost");
        node.ws_port = ReadOptionalValue(getter, section, "WsPort");
        node.ws_path = ReadOptionalValue(getter, section, "WsPath");
        node.ws_tls = ParseOptionalFlag(getter(section, "WsTls"), false);
        node.wt_enabled = ParseOptionalFlag(getter(section, "WtEnabled"), false);
        node.wt_host = ReadOptionalValue(getter, section, "WtHost");
        node.wt_port = ReadOptionalValue(getter, section, "WtPort");
        node.wt_path = ReadOptionalValue(getter, section, "WtPath");
        node.wt_tls = ParseOptionalFlag(getter(section, "WtTls"), false);
        if (!ReadRequiredValue(getter, section, "RpcHost", node.rpc_host, error) ||
            !ReadRequiredValue(getter, section, "RpcPort", node.rpc_port, error))
        {
            return false;
        }
        node.enabled = ParseEnabledFlag(getter(section, "Enabled"));

        if (!seen_names.insert(node.name).second)
        {
            error = "duplicate chat node name: " + node.name;
            return false;
        }

        const auto tcp_endpoint = std::make_pair(node.tcp_host, node.tcp_port);
        if (!tcp_endpoints.insert(tcp_endpoint).second)
        {
            error = "duplicate chat tcp endpoint: " + node.tcp_host + ":" + node.tcp_port;
            return false;
        }

        if (!node.quic_host.empty() && !node.quic_port.empty())
        {
            const auto quic_endpoint = std::make_pair(node.quic_host, node.quic_port);
            if (!quic_endpoints.insert(quic_endpoint).second)
            {
                error = "duplicate chat quic endpoint: " + node.quic_host + ":" + node.quic_port;
                return false;
            }
        }

        if (node.ws_enabled)
        {
            if (node.ws_host.empty())
            {
                node.ws_host = node.tcp_host;
            }
            if (node.ws_path.empty())
            {
                node.ws_path = "/ws";
            }
            if (node.ws_port.empty())
            {
                error = "missing enabled websocket endpoint port: " + node.name;
                return false;
            }
            const auto ws_endpoint = std::make_pair(node.ws_host, node.ws_port);
            if (!ws_endpoints.insert(ws_endpoint).second)
            {
                error = "duplicate chat websocket endpoint: " + node.ws_host + ":" + node.ws_port;
                return false;
            }
        }

        if (node.wt_enabled)
        {
            if (node.wt_host.empty())
            {
                node.wt_host = node.tcp_host;
            }
            if (node.wt_path.empty())
            {
                node.wt_path = "/chat";
            }
            if (node.wt_port.empty())
            {
                error = "missing enabled webtransport endpoint port: " + node.name;
                return false;
            }
            const auto wt_endpoint = std::make_pair(node.wt_host, node.wt_port);
            if (!wt_endpoints.insert(wt_endpoint).second)
            {
                error = "duplicate chat webtransport endpoint: " + node.wt_host + ":" + node.wt_port;
                return false;
            }
        }

        const auto rpc_endpoint = std::make_pair(node.rpc_host, node.rpc_port);
        if (!rpc_endpoints.insert(rpc_endpoint).second)
        {
            error = "duplicate chat rpc endpoint: " + node.rpc_host + ":" + node.rpc_port;
            return false;
        }

        config.nodes.push_back(node);
    }

    if (config.enabledNodes().empty())
    {
        error = "cluster config has no enabled chat nodes";
        return false;
    }

    if (!self_node_name.empty())
    {
        const ChatNodeDescriptor* self_node = config.findNode(self_node_name);
        if (self_node == nullptr)
        {
            error = "self chat node missing from cluster config: " + self_node_name;
            return false;
        }
        if (!self_node->enabled)
        {
            error = "self chat node is disabled in cluster config: " + self_node_name;
            return false;
        }
    }

    out = std::move(config);
    return true;
}

inline std::string GetEnvOrEmpty(const char* name)
{
    if (name == nullptr || *name == '\0')
    {
        return "";
    }
    const char* value = std::getenv(name);
    return value == nullptr ? "" : std::string(value);
}

inline int ParsePositiveIntOrDefault(const std::string& raw, int fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    int value = 0;
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == raw.data() + raw.size() && value > 0 ? value : fallback;
}

inline std::string ResolveSelfNodeName(const ConfigValueGetter& getter)
{
    std::string self_name = TrimCopy(GetEnvOrEmpty("MEMOCHAT_INSTANCE_NAME"));
    if (!self_name.empty())
    {
        return self_name;
    }

    self_name = TrimCopy(GetEnvOrEmpty("MEMOCHAT_POD_NAME"));
    if (!self_name.empty())
    {
        return self_name;
    }

    self_name = TrimCopy(GetEnvOrEmpty("HOSTNAME"));
    if (!self_name.empty())
    {
        return self_name;
    }

    if (getter)
    {
        self_name = TrimCopy(getter("SelfServer", "Name"));
    }
    return self_name;
}

inline bool LoadK8sStatefulSetChatClusterConfig(const ConfigValueGetter& getter,
                                                const std::string& self_node_name,
                                                ChatClusterConfig& out,
                                                std::string& error)
{
    error.clear();
    if (!getter)
    {
        error = "cluster config getter is not set";
        return false;
    }

    ChatClusterConfig config;
    config.discovery_mode = "k8s-statefulset";

    std::string service_name;
    std::string namespace_name;
    std::string replicas_value;
    std::string tcp_port;
    std::string rpc_port;
    if (!ReadRequiredValue(getter, "Cluster", "ServiceName", service_name, error) ||
        !ReadRequiredValue(getter, "Cluster", "Namespace", namespace_name, error) ||
        !ReadRequiredValue(getter, "Cluster", "Replicas", replicas_value, error) ||
        !ReadRequiredValue(getter, "Cluster", "TcpPort", tcp_port, error) ||
        !ReadRequiredValue(getter, "Cluster", "RpcPort", rpc_port, error))
    {
        return false;
    }
    const int replicas = ParsePositiveIntOrDefault(replicas_value, 1);
    const std::string quic_port = ReadOptionalValue(getter, "Cluster", "QuicPort");
    const std::string domain = TrimCopy(getter("Cluster", "Domain"));
    const std::string cluster_domain = domain.empty() ? "cluster.local" : domain;

    std::unordered_set<std::string> seen_names;
    for (int ordinal = 0; ordinal < replicas; ++ordinal)
    {
        ChatNodeDescriptor node;
        node.name = service_name + "-" + std::to_string(ordinal);
        node.tcp_host = node.name + "." + service_name + "." + namespace_name + ".svc." + cluster_domain;
        node.tcp_port = tcp_port;
        node.quic_host = quic_port.empty() ? "" : node.tcp_host;
        node.quic_port = quic_port;
        node.ws_enabled = false;
        node.wt_enabled = false;
        node.rpc_host = node.tcp_host;
        node.rpc_port = rpc_port;
        node.enabled = true;
        if (!seen_names.insert(node.name).second)
        {
            error = "duplicate k8s chat node name: " + node.name;
            return false;
        }
        config.nodes.push_back(node);
    }

    if (config.nodes.empty())
    {
        error = "k8s cluster config has no nodes";
        return false;
    }

    const std::string resolved_self = self_node_name.empty() ? ResolveSelfNodeName(getter) : self_node_name;
    if (!resolved_self.empty() && config.findNode(resolved_self) == nullptr)
    {
        error = "self chat node missing from k8s cluster config: " + resolved_self;
        return false;
    }

    out = std::move(config);
    return true;
}

inline bool LoadChatClusterConfig(const ConfigValueGetter& getter,
                                  const std::string& self_node_name,
                                  ChatClusterConfig& out,
                                  std::string& error)
{
    error.clear();
    if (!getter)
    {
        error = "cluster config getter is not set";
        return false;
    }

    const std::string discovery_mode = TrimCopy(getter("Cluster", "DiscoveryMode"));
    if (discovery_mode == "k8s-statefulset")
    {
        return LoadK8sStatefulSetChatClusterConfig(getter, self_node_name, out, error);
    }
    if (discovery_mode == "etcd")
    {
        error = "etcd discovery mode requires EtcdClusterDiscovery class";
        return false;
    }
    return LoadStaticChatClusterConfig(getter, self_node_name, out, error);
}

} // namespace memochat::cluster
