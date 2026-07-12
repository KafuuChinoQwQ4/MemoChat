#include "runtime/EtcdConfig.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

import memochat.runtime.etcd_config_algorithms;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace memochat::runtime
{
namespace
{
struct ParsedEndpoint
{
    std::string host;
    std::string port;
};

ParsedEndpoint ParseEndpoint(const std::string& endpoints)
{
    const size_t comma_pos = endpoints.find(',');
    std::string endpoint =
        modules::HasMultipleEndpoints(comma_pos != std::string::npos) ? endpoints.substr(0, comma_pos) : endpoints;

    const size_t scheme_pos = endpoint.find("://");
    std::string host = modules::HasSchemeDelimiter(scheme_pos != std::string::npos)
                           ? endpoint.substr(scheme_pos + modules::SchemeDelimiterSize())
                           : endpoint;
    const size_t port_colon = host.find(':');
    if (!modules::HasExplicitPort(port_colon != std::string::npos))
    {
        return {std::move(host), modules::DefaultEtcdPort()};
    }
    std::string port = host.substr(port_colon + 1);
    host.resize(port_colon);
    return {std::move(host), std::move(port)};
}

bool ConnectSocket(const ParsedEndpoint& endpoint, net::io_context& ioc, tcp::socket& socket, std::string& error)
{
    tcp::resolver resolver(ioc);
    boost::system::error_code ec;
    const auto resolved = resolver.resolve(endpoint.host, endpoint.port, ec);
    if (ec)
    {
        error = ec.message();
        return false;
    }
    net::connect(socket, resolved, ec);
    if (ec)
    {
        error = ec.message();
        return false;
    }
    return true;
}

bool PostEtcd(const std::string& endpoints,
              const std::string& target,
              const std::string& body,
              std::string& response,
              std::string& error)
{
    const ParsedEndpoint endpoint = ParseEndpoint(endpoints);
    if (endpoint.host.empty() || endpoint.port.empty())
    {
        error = "invalid etcd endpoint";
        return false;
    }

    net::io_context ioc;
    tcp::socket socket(ioc);
    if (!ConnectSocket(endpoint, ioc, socket, error))
    {
        return false;
    }

    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, endpoint.host);
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();

    boost::system::error_code ec;
    http::write(socket, req, ec);
    if (ec)
    {
        error = ec.message();
        return false;
    }

    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(socket, buffer, res, ec);
    if (ec)
    {
        error = ec.message();
        return false;
    }

    response = beast::buffers_to_string(res.body().data());
    socket.close(ec);
    return true;
}
} // namespace

EtcdClient::EtcdClient(const std::string& endpoints)
    : _endpoints(endpoints)
{
    _running.store(true);
    if (!_connect_thread.Start(
            [this]() noexcept
            {
                ConnectLoop();
            },
            &_startup_error))
    {
        _running.store(false);
    }
}

EtcdClient::~EtcdClient()
{
    Stop();
}

void EtcdClient::Stop()
{
    _running.store(false);
    std::string error;
    if (_connect_thread.Joinable() && !_connect_thread.Join(&error))
    {
        std::cerr << "[EtcdClient] Failed to join connect thread: " << error << std::endl;
    }
    if (_watch_thread.Joinable() && !_watch_thread.Join(&error))
    {
        std::cerr << "[EtcdClient] Failed to join watch thread: " << error << std::endl;
    }
}

void EtcdClient::ConnectLoop()
{
    while (_running.load())
    {
        if (DoConnect())
        {
            _connected.store(true);
            std::cout << "[EtcdClient] Connected to etcd: " << _endpoints << std::endl;
            WatchLoop();
        }
        _connected.store(false);
        for (int i = 0; i < 5 && _running.load(); ++i)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

bool EtcdClient::DoConnect()
{
    const ParsedEndpoint endpoint = ParseEndpoint(_endpoints);
    net::io_context ioc;
    tcp::socket socket(ioc);
    std::string error;
    if (!ConnectSocket(endpoint, ioc, socket, error))
    {
        std::cerr << "[EtcdClient] Connection failed: " << error << std::endl;
        return false;
    }
    boost::system::error_code ec;
    socket.close(ec);
    return true;
}

bool EtcdClient::Get(const std::string& key, std::string& value)
{
    if (!_connected.load())
    {
        return false;
    }

    std::ostringstream oss;
    oss << R"({"key":")" << key << R"("})";
    std::string response;
    std::string error;
    if (!PostEtcd(_endpoints, "/v3/kv/range", oss.str(), response, error))
    {
        std::cerr << "[EtcdClient] Get failed: " << error << std::endl;
        return false;
    }
    if (modules::IsResponseWithKvs(response.find("\"kvs\"") != std::string::npos))
    {
        size_t value_start = response.find("\"value\":\"");
        if (value_start != std::string::npos)
        {
            value_start += 9;
            const size_t value_end = response.find('"', value_start);
            if (value_end != std::string::npos)
            {
                value = response.substr(value_start, value_end - value_start);
                return true;
            }
        }
    }
    return false;
}

bool EtcdClient::Set(const std::string& key, const std::string& value, int64_t ttl)
{
    if (!_connected.load())
    {
        return false;
    }

    std::ostringstream oss;
    oss << R"({"key":")" << key << R"(","value":")" << value << R"(")";
    if (modules::ShouldAttachLease(ttl > 0))
    {
        oss << R"(,"lease":")" << ttl << R"(")";
    }
    oss << "}";
    std::string response;
    std::string error;
    if (!PostEtcd(_endpoints, "/v3/kv/put", oss.str(), response, error))
    {
        std::cerr << "[EtcdClient] Set failed: " << error << std::endl;
        return false;
    }
    return modules::IsResponseWithHeader(response.find("\"header\"") != std::string::npos);
}

bool EtcdClient::Delete(const std::string& key)
{
    if (!_connected.load())
    {
        return false;
    }

    std::ostringstream oss;
    oss << R"({"key":")" << key << R"("})";
    std::string response;
    std::string error;
    if (!PostEtcd(_endpoints, "/v3/kv/deleterange", oss.str(), response, error))
    {
        std::cerr << "[EtcdClient] Delete failed: " << error << std::endl;
        return false;
    }
    return modules::IsResponseWithHeader(response.find("\"header\"") != std::string::npos);
}

bool EtcdClient::Watch(const std::string& prefix, WatchCallback callback)
{
    std::lock_guard<std::mutex> lock(_watch_mutex);
    _watch_callbacks.push_back(callback);

    if (modules::ShouldStartWatchThread(_watch_thread.Joinable()))
    {
        std::string error;
        if (!_watch_thread.Start(
                [this]() noexcept
                {
                    WatchLoop();
                },
                &error))
        {
            std::cerr << "[EtcdClient] Failed to start watch thread: " << error << std::endl;
            _watch_callbacks.pop_back();
            return false;
        }
    }
    return true;
}

void EtcdClient::WatchLoop()
{
    std::cout << "[EtcdClient] Watch loop started" << std::endl;

    while (_running.load() && _connected.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[EtcdClient] Watch loop stopped" << std::endl;
}

std::vector<std::pair<std::string, std::string>> EtcdClient::GetAll(const std::string& prefix)
{
    std::vector<std::pair<std::string, std::string>> results;

    if (!_connected.load())
    {
        return results;
    }

    std::ostringstream oss;
    oss << R"({"key":")" << prefix << R"(","range_end":")" << prefix << R"(0"})";
    std::string response;
    std::string error;
    if (!PostEtcd(_endpoints, "/v3/kv/range", oss.str(), response, error))
    {
        std::cerr << "[EtcdClient] GetAll failed: " << error << std::endl;
        return results;
    }

    const size_t kvs_pos = response.find("\"kvs\":[");
    if (modules::IsResponseWithKvs(kvs_pos != std::string::npos))
    {
        std::string kvs_json = response.substr(kvs_pos + 7);
        size_t pos = 0;
        while ((pos = kvs_json.find("\"key\":\"", pos)) != std::string::npos)
        {
            const size_t key_start = pos + 7;
            const size_t key_end = kvs_json.find('"', key_start);
            if (key_end == std::string::npos)
                break;
            std::string key = kvs_json.substr(key_start, key_end - key_start);

            const size_t value_pos = kvs_json.find("\"value\":\"", key_end);
            if (value_pos == std::string::npos)
                break;
            const size_t value_start = value_pos + 9;
            const size_t value_end = kvs_json.find('"', value_start);
            if (value_end == std::string::npos)
                break;

            std::string value = kvs_json.substr(value_start, value_end - value_start);
            results.emplace_back(std::move(key), std::move(value));
            pos = value_end;
        }
    }

    return results;
}

std::string EtcdConfig::BuildKey(const std::string& service_name, const std::string& section, const std::string& key)
{
    return "/memochat/" + service_name + "/" + section + "/" + key;
}

EtcdConfig::EtcdConfig(const std::string& endpoints, const std::string& service_name)
    : _client(std::make_shared<EtcdClient>(endpoints))
    , _service_name(service_name)
    , _config_prefix("/memochat/" + service_name + "/")
{
}

EtcdConfig::~EtcdConfig()
{
    Stop();
}

bool EtcdConfig::Get(const std::string& section, const std::string& key, std::string& value)
{
    const std::string full_key = BuildKey(_service_name, section, key);

    std::lock_guard<std::mutex> lock(_config_mutex);
    auto section_it = _config_cache.find(section);
    if (section_it != _config_cache.end())
    {
        auto key_it = section_it->second.find(key);
        if (key_it != section_it->second.end())
        {
            value = key_it->second;
            return true;
        }
    }

    if (_client->Get(full_key, value))
    {
        _config_cache[section][key] = value;
        return true;
    }
    return false;
}

bool EtcdConfig::Set(const std::string& section, const std::string& key, const std::string& value, int64_t ttl)
{
    if (!_client)
    {
        return false;
    }
    const std::string full_key = key.starts_with('/') ? key : BuildKey(_service_name, section, key);
    if (!_client->Set(full_key, value, ttl))
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(_config_mutex);
    _config_cache[section][key] = value;
    return true;
}

bool EtcdConfig::Delete(const std::string& section, const std::string& key)
{
    if (!_client)
    {
        return false;
    }
    const std::string full_key = key.starts_with('/') ? key : BuildKey(_service_name, section, key);
    if (!_client->Delete(full_key))
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(_config_mutex);
    if (auto section_it = _config_cache.find(section); section_it != _config_cache.end())
    {
        section_it->second.erase(key);
    }
    return true;
}

std::vector<std::pair<std::string, std::string>> EtcdConfig::GetAll(const std::string& prefix)
{
    return _client ? _client->GetAll(prefix) : std::vector<std::pair<std::string, std::string>>{};
}

void EtcdConfig::SetChangeCallback(ChangeCallback callback)
{
    _change_callback = callback;
}

void EtcdConfig::StartWatch()
{
    if (modules::ShouldSkipConfigWatchStart(_watching.load()))
    {
        return;
    }

    _watching.store(true);

    _client->Watch(_config_prefix,
                   [this](const std::string& key, const std::string& value)
                   {
                       OnConfigChange(key, value);
                   });
}

void EtcdConfig::Stop()
{
    _watching.store(false);
    if (_client)
    {
        _client->Stop();
    }
}

void EtcdConfig::OnConfigChange(const std::string& key, const std::string& value)
{
    auto [section, config_key] = ParseKey(key);
    if (!modules::ShouldAcceptConfigChangeKey(section.empty(), config_key.empty()))
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_config_mutex);
        _config_cache[section][config_key] = value;
    }

    if (_change_callback)
    {
        _change_callback(section, config_key, value);
    }
}

std::pair<std::string, std::string> EtcdConfig::ParseKey(const std::string& key)
{
    std::pair<std::string, std::string> result;

    if (!modules::HasConfigPrefix(key.find(_config_prefix) == 0))
    {
        return result;
    }

    std::string remaining = key.substr(_config_prefix.size());
    size_t first_slash = remaining.find('/');
    if (!modules::HasConfigSectionSeparator(first_slash != std::string::npos))
    {
        return result;
    }

    result.first = remaining.substr(0, first_slash);
    result.second = remaining.substr(first_slash + 1);

    return result;
}

std::shared_ptr<EtcdConfig> EtcdConfigLoader::TryCreate(const std::string& endpoints, const std::string& service_name)
{
    if (!modules::ShouldCreateEtcdConfig(endpoints.empty()))
    {
        std::cout << "[EtcdConfigLoader] No etcd endpoints configured" << std::endl;
        return nullptr;
    }

    auto config = std::make_shared<EtcdConfig>(endpoints, service_name);

    std::string test_value;
    if (!config->startupError().empty())
    {
        std::cout << "[EtcdConfigLoader] Failed to start etcd client: " << config->startupError() << std::endl;
        return nullptr;
    }
    if (!config->IsAvailable())
    {
        std::cout << "[EtcdConfigLoader] Failed to connect to etcd: " << endpoints << std::endl;
        return nullptr;
    }

    std::cout << "[EtcdConfigLoader] Successfully connected to etcd: " << endpoints << std::endl;
    return config;
}

} // namespace memochat::runtime
