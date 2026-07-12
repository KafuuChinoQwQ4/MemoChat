#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "runtime/ExplicitThread.hpp"

namespace memochat::runtime
{

class EtcdClient
{
public:
    using WatchCallback = std::function<void(const std::string& key, const std::string& value)>;

    explicit EtcdClient(const std::string& endpoints);
    ~EtcdClient();

    bool IsConnected() const
    {
        return _connected.load();
    }

    const std::string& startupError() const noexcept
    {
        return _startup_error;
    }

    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value, int64_t ttl = 0);
    bool Delete(const std::string& key);
    bool Watch(const std::string& prefix, WatchCallback callback);
    std::vector<std::pair<std::string, std::string>> GetAll(const std::string& prefix);

    void Stop();

private:
    void ConnectLoop();
    bool DoConnect();
    void WatchLoop();

    std::string _endpoints;
    std::atomic<bool> _connected{false};
    std::atomic<bool> _running{false};

    std::mutex _watch_mutex;
    std::vector<WatchCallback> _watch_callbacks;
    ExplicitThread _watch_thread;
    ExplicitThread _connect_thread;
    std::string _startup_error;
};

class EtcdConfig
{
public:
    using ChangeCallback =
        std::function<void(const std::string& section, const std::string& key, const std::string& value)>;

    explicit EtcdConfig(const std::string& endpoints, const std::string& service_name);
    ~EtcdConfig();

    bool IsAvailable() const
    {
        return _client && _client->IsConnected();
    }

    const std::string& startupError() const noexcept
    {
        return _client->startupError();
    }

    bool Get(const std::string& section, const std::string& key, std::string& value);
    bool Set(const std::string& section, const std::string& key, const std::string& value, int64_t ttl = 0);
    bool Delete(const std::string& section, const std::string& key);
    std::vector<std::pair<std::string, std::string>> GetAll(const std::string& prefix);
    void SetChangeCallback(ChangeCallback callback);

    void StartWatch();
    void Stop();

    static std::string BuildKey(const std::string& service_name, const std::string& section, const std::string& key);

private:
    void OnConfigChange(const std::string& key, const std::string& value);
    std::pair<std::string, std::string> ParseKey(const std::string& key);

    std::shared_ptr<EtcdClient> _client;
    std::string _service_name;
    std::string _config_prefix;

    std::mutex _config_mutex;
    std::map<std::string, std::map<std::string, std::string>> _config_cache;

    ChangeCallback _change_callback;
    std::atomic<bool> _watching{false};
};

class EtcdConfigLoader
{
public:
    static std::shared_ptr<EtcdConfig> TryCreate(const std::string& endpoints, const std::string& service_name);
};

} // namespace memochat::runtime
