#include "runtime/EtcdConfig.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <sstream>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace memochat::runtime {

EtcdClient::EtcdClient(const std::string& endpoints)
    : _endpoints(endpoints) {
    _running.store(true);
    _connect_thread = std::thread(&EtcdClient::ConnectLoop, this);
}

EtcdClient::~EtcdClient() {
    Stop();
}

void EtcdClient::Stop() {
    _running.store(false);
    if (_connect_thread.joinable()) {
        _connect_thread.join();
    }
    if (_watch_thread.joinable()) {
        _watch_thread.join();
    }
}

void EtcdClient::ConnectLoop() {
    while (_running.load()) {
        if (DoConnect()) {
            _connected.store(true);
            std::cout << "[EtcdClient] Connected to etcd: " << _endpoints << std::endl;
            WatchLoop();
        }
        _connected.store(false);
        for (int i = 0; i < 5 && _running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

bool EtcdClient::DoConnect() {
    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);

        size_t comma_pos = _endpoints.find(',');
        std::string endpoint = comma_pos != std::string::npos
            ? _endpoints.substr(0, comma_pos)
            : _endpoints;

        std::string host, port;
        size_t colon_pos = endpoint.find("://");
        if (colon_pos != std::string::npos) {
            host = endpoint.substr(colon_pos + 3);
        } else {
            host = endpoint;
        }

        size_t port_colon = host.find(':');
        if (port_colon != std::string::npos) {
            port = host.substr(port_colon + 1);
            host = host.substr(0, port_colon);
        } else {
            port = "2379";
        }

        auto results = resolver.resolve(host, port);
        tcp::socket socket(ioc);
        net::connect(socket, results);

        socket.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[EtcdClient] Connection failed: " << e.what() << std::endl;
        return false;
    }
}

bool EtcdClient::Get(const std::string& key, std::string& value) {
    if (!_connected.load()) {
        return false;
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);

        size_t comma_pos = _endpoints.find(',');
        std::string endpoint = comma_pos != std::string::npos
            ? _endpoints.substr(0, comma_pos)
            : _endpoints;

        std::string host, port;
        size_t colon_pos = endpoint.find("://");
        if (colon_pos != std::string::npos) {
            host = endpoint.substr(colon_pos + 3);
        } else {
            host = endpoint;
        }

        size_t port_colon = host.find(':');
        if (port_colon != std::string::npos) {
            port = host.substr(port_colon + 1);
            host = host.substr(0, port_colon);
        } else {
            port = "2379";
        }

        auto results = resolver.resolve(host, port);
        tcp::socket socket(ioc);
        net::connect(socket, results);

        std::ostringstream oss;
        oss << R"({"key":")" << key << R"("})";
        std::string body = oss.str();

        http::request<http::string_body> req{http::verb::post, "/v3/kv/range", 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(socket, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        if (response.find("\"kvs\"") != std::string::npos) {
            size_t value_start = response.find("\"value\":\"");
            if (value_start != std::string::npos) {
                value_start += 9;
                size_t value_end = response.find("\"", value_start);
                value = response.substr(value_start, value_end - value_start);
                return true;
            }
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[EtcdClient] Get failed: " << e.what() << std::endl;
        return false;
    }
}

bool EtcdClient::Set(const std::string& key, const std::string& value, int64_t ttl) {
    if (!_connected.load()) {
        return false;
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);

        size_t comma_pos = _endpoints.find(',');
        std::string endpoint = comma_pos != std::string::npos
            ? _endpoints.substr(0, comma_pos)
            : _endpoints;

        std::string host, port;
        size_t colon_pos = endpoint.find("://");
        if (colon_pos != std::string::npos) {
            host = endpoint.substr(colon_pos + 3);
        } else {
            host = endpoint;
        }

        size_t port_colon = host.find(':');
        if (port_colon != std::string::npos) {
            port = host.substr(port_colon + 1);
            host = host.substr(0, port_colon);
        } else {
            port = "2379";
        }

        auto results = resolver.resolve(host, port);
        tcp::socket socket(ioc);
        net::connect(socket, results);

        std::ostringstream oss;
        oss << R"({"key":")" << key << R"(","value":")" << value << R"(")";
        if (ttl > 0) {
            oss << R"(,"lease":")" << ttl << R"(")";
        }
        oss << "}";
        std::string body = oss.str();

        http::request<http::string_body> req{http::verb::post, "/v3/kv/put", 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(socket, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        return response.find("\"header\"") != std::string::npos;
    } catch (const std::exception& e) {
        std::cerr << "[EtcdClient] Set failed: " << e.what() << std::endl;
        return false;
    }
}

bool EtcdClient::Delete(const std::string& key) {
    if (!_connected.load()) {
        return false;
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);

        size_t comma_pos = _endpoints.find(',');
        std::string endpoint = comma_pos != std::string::npos
            ? _endpoints.substr(0, comma_pos)
            : _endpoints;

        std::string host, port;
        size_t colon_pos = endpoint.find("://");
        if (colon_pos != std::string::npos) {
            host = endpoint.substr(colon_pos + 3);
        } else {
            host = endpoint;
        }

        size_t port_colon = host.find(':');
        if (port_colon != std::string::npos) {
            port = host.substr(port_colon + 1);
            host = host.substr(0, port_colon);
        } else {
            port = "2379";
        }

        auto results = resolver.resolve(host, port);
        tcp::socket socket(ioc);
        net::connect(socket, results);

        std::ostringstream oss;
        oss << R"({"key":")" << key << R"("})";
        std::string body = oss.str();

        http::request<http::string_body> req{http::verb::post, "/v3/kv/deleterange", 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(socket, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());
        return response.find("\"header\"") != std::string::npos;
    } catch (const std::exception& e) {
        std::cerr << "[EtcdClient] Delete failed: " << e.what() << std::endl;
        return false;
    }
}

bool EtcdClient::Watch(const std::string& prefix, WatchCallback callback) {
    std::lock_guard<std::mutex> lock(_watch_mutex);
    _watch_callbacks.push_back(callback);

    if (!_watch_thread.joinable()) {
        _watch_thread = std::thread(&EtcdClient::WatchLoop, this);
    }
    return true;
}

void EtcdClient::WatchLoop() {
    std::cout << "[EtcdClient] Watch loop started" << std::endl;

    while (_running.load() && _connected.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[EtcdClient] Watch loop stopped" << std::endl;
}

std::vector<std::pair<std::string, std::string>> EtcdClient::GetAll(const std::string& prefix) {
    std::vector<std::pair<std::string, std::string>> results;

    if (!_connected.load()) {
        return results;
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);

        size_t comma_pos = _endpoints.find(',');
        std::string endpoint = comma_pos != std::string::npos
            ? _endpoints.substr(0, comma_pos)
            : _endpoints;

        std::string host, port;
        size_t colon_pos = endpoint.find("://");
        if (colon_pos != std::string::npos) {
            host = endpoint.substr(colon_pos + 3);
        } else {
            host = endpoint;
        }

        size_t port_colon = host.find(':');
        if (port_colon != std::string::npos) {
            port = host.substr(port_colon + 1);
            host = host.substr(0, port_colon);
        } else {
            port = "2379";
        }

        auto resolved = resolver.resolve(host, port);
        tcp::socket socket(ioc);
        net::connect(socket, resolved);

        std::ostringstream oss;
        oss << R"({"key":")" << prefix << R"(","range_end":")" << prefix << R"(0"})";
        std::string body = oss.str();

        http::request<http::string_body> req{http::verb::post, "/v3/kv/range", 11};
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(socket, buffer, res);

        std::string response = beast::buffers_to_string(res.body().data());

        size_t kvs_pos = response.find("\"kvs\":[");
        if (kvs_pos != std::string::npos) {
            std::string kvs_json = response.substr(kvs_pos + 7);
            size_t pos = 0;
            while ((pos = kvs_json.find("\"key\":\"", pos)) != std::string::npos) {
                size_t key_start = pos + 7;
                size_t key_end = kvs_json.find("\"", key_start);
                if (key_end == std::string::npos) break;
                std::string key = kvs_json.substr(key_start, key_end - key_start);

                size_t value_pos = kvs_json.find("\"value\":\"", key_end);
                if (value_pos == std::string::npos) break;
                size_t value_start = value_pos + 9;
                size_t value_end = kvs_json.find("\"", value_start);
                if (value_end == std::string::npos) break;

                std::string value = kvs_json.substr(value_start, value_end - value_start);
                results.emplace_back(key, value);

                pos = value_end;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[EtcdClient] GetAll failed: " << e.what() << std::endl;
    }

    return results;
}

std::string EtcdConfig::BuildKey(const std::string& service_name, const std::string& section, const std::string& key) {
    return "/memochat/" + service_name + "/" + section + "/" + key;
}

EtcdConfig::EtcdConfig(const std::string& endpoints, const std::string& service_name)
    : _client(std::make_shared<EtcdClient>(endpoints))
    , _service_name(service_name)
    , _config_prefix("/memochat/" + service_name + "/") {
}

EtcdConfig::~EtcdConfig() {
    Stop();
}

bool EtcdConfig::Get(const std::string& section, const std::string& key, std::string& value) {
    const std::string full_key = BuildKey(_service_name, section, key);

    std::lock_guard<std::mutex> lock(_config_mutex);
    auto section_it = _config_cache.find(section);
    if (section_it != _config_cache.end()) {
        auto key_it = section_it->second.find(key);
        if (key_it != section_it->second.end()) {
            value = key_it->second;
            return true;
        }
    }

    if (_client->Get(full_key, value)) {
        _config_cache[section][key] = value;
        return true;
    }
    return false;
}

void EtcdConfig::SetChangeCallback(ChangeCallback callback) {
    _change_callback = callback;
}

void EtcdConfig::StartWatch() {
    if (_watching.load()) {
        return;
    }

    _watching.store(true);

    _client->Watch(_config_prefix, [this](const std::string& key, const std::string& value) {
        OnConfigChange(key, value);
    });
}

void EtcdConfig::Stop() {
    _watching.store(false);
    if (_client) {
        _client->Stop();
    }
}

void EtcdConfig::OnConfigChange(const std::string& key, const std::string& value) {
    auto [section, config_key] = ParseKey(key);
    if (section.empty() || config_key.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_config_mutex);
        _config_cache[section][config_key] = value;
    }

    if (_change_callback) {
        _change_callback(section, config_key, value);
    }
}

std::pair<std::string, std::string> EtcdConfig::ParseKey(const std::string& key) {
    std::pair<std::string, std::string> result;

    if (key.find(_config_prefix) != 0) {
        return result;
    }

    std::string remaining = key.substr(_config_prefix.size());
    size_t first_slash = remaining.find('/');
    if (first_slash == std::string::npos) {
        return result;
    }

    result.first = remaining.substr(0, first_slash);
    result.second = remaining.substr(first_slash + 1);

    return result;
}

std::shared_ptr<EtcdConfig> EtcdConfigLoader::TryCreate(const std::string& endpoints, const std::string& service_name) {
    if (endpoints.empty()) {
        std::cout << "[EtcdConfigLoader] No etcd endpoints configured" << std::endl;
        return nullptr;
    }

    auto config = std::make_shared<EtcdConfig>(endpoints, service_name);

    std::string test_value;
    if (!config->IsAvailable()) {
        std::cout << "[EtcdConfigLoader] Failed to connect to etcd: " << endpoints << std::endl;
        return nullptr;
    }

    std::cout << "[EtcdConfigLoader] Successfully connected to etcd: " << endpoints << std::endl;
    return config;
}

} // namespace memochat::runtime
