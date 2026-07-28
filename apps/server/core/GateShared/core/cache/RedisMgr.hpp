#pragma once
#include "const.hpp"
#include <hiredis/hiredis.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include "Singleton.hpp"
#include "runtime/ExplicitThread.hpp"
class RedisConPool
{
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : b_stop_(false)
        , poolSize_(poolSize)
        , host_(host == nullptr ? "" : host)
        , pwd_(pwd == nullptr ? "" : pwd)
        , port_(port)
    {
        for (size_t i = 0; i < poolSize_; ++i)
        {
            auto* context = ConnectAuthenticated(false);
            if (context != nullptr)
            {
                connections_.push(context);
                ++connection_count_;
            }
        }
        health_context_ = ConnectAuthenticated(true);

        if (connections_.empty())
        {
            startup_error_ = "redis pool has no usable connections";
            return;
        }

        ready_ = true;
        healthy_.store(true, std::memory_order_release);

        std::string thread_error;
        if (!check_thread_.Start(
                [this]()
                {
                    while (!b_stop_.load(std::memory_order_acquire))
                    {
                        {
                            std::unique_lock<std::mutex> lock(mutex_);
                            if (cond_.wait_for(lock,
                                               kHealthCheckInterval,
                                               [this]
                                               {
                                                   return b_stop_.load(std::memory_order_acquire);
                                               }))
                            {
                                break;
                            }
                        }
                        CheckAndRepairConnections();
                    }
                },
                &thread_error))
        {
            ready_ = false;
            healthy_.store(false, std::memory_order_release);
            startup_error_ = "redis pool checker init failed: " + thread_error;
            return;
        }
    }

    [[nodiscard]] bool Ready() const noexcept
    {
        return ready_;
    }

    [[nodiscard]] const std::string& StartupError() const noexcept
    {
        return startup_error_;
    }

    [[nodiscard]] bool Healthy() const noexcept
    {
        return ready_ && healthy_.load(std::memory_order_acquire);
    }

    ~RedisConPool()
    {
        Close();
        ClearConnections();
    }

    void ClearConnections()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty())
        {
            auto* context = connections_.front();
            redisFree(context);
            connections_.pop();
        }
        connection_count_ = 0;
        if (health_context_ != nullptr)
        {
            redisFree(health_context_);
            health_context_ = nullptr;
        }
    }

    redisContext* getConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock,
                   [this]
                   {
                       if (b_stop_.load(std::memory_order_acquire) || !ready_ ||
                           !healthy_.load(std::memory_order_acquire))
                       {
                           return true;
                       }
                       return !connections_.empty();
                   });

        if (b_stop_.load(std::memory_order_acquire) || !ready_ || !healthy_.load(std::memory_order_acquire))
        {
            return nullptr;
        }
        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    redisContext* getConNonBlock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_.load(std::memory_order_acquire) || !ready_)
        {
            return nullptr;
        }

        if (connections_.empty())
        {
            return nullptr;
        }

        auto* context = connections_.front();
        connections_.pop();
        return context;
    }

    void returnConnection(redisContext* context)
    {
        if (context == nullptr)
        {
            return;
        }
        if (context->err != 0)
        {
            returnBrokenConnection(context);
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_.load(std::memory_order_acquire))
        {
            redisFree(context);
            if (connection_count_ > 0)
            {
                --connection_count_;
            }
            return;
        }
        connections_.push(context);
        cond_.notify_one();
    }

    void returnBrokenConnection(redisContext* context)
    {
        if (context == nullptr)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        redisFree(context);
        if (connection_count_ > 0)
        {
            --connection_count_;
        }
        if (connection_count_ == 0)
        {
            healthy_.store(false, std::memory_order_release);
        }
        cond_.notify_all();
    }

    void Close()
    {
        if (b_stop_.exchange(true))
        {
            return;
        }
        healthy_.store(false, std::memory_order_release);
        cond_.notify_all();
        if (check_thread_.Joinable())
        {
            std::string thread_error;
            if (!check_thread_.Join(&thread_error))
            {
                std::cout << "redis pool checker join failed, error is " << thread_error << std::endl;
            }
        }
    }

private:
    static constexpr std::chrono::seconds kHealthCheckInterval{5};
    static constexpr long kRedisIoTimeoutSeconds = 1;

    static bool SetIoTimeout(redisContext* context, bool bounded)
    {
        const timeval timeout{.tv_sec = bounded ? kRedisIoTimeoutSeconds : 0, .tv_usec = 0};
        return redisSetTimeout(context, timeout) == REDIS_OK;
    }

    redisContext* ConnectAuthenticated(bool keep_io_timeout) const
    {
        const timeval timeout{.tv_sec = kRedisIoTimeoutSeconds, .tv_usec = 0};
        auto* context = redisConnectWithTimeout(host_.c_str(), port_, timeout);
        if (context == nullptr || context->err != 0)
        {
            if (context != nullptr)
            {
                redisFree(context);
            }
            return nullptr;
        }

        if (!SetIoTimeout(context, true))
        {
            redisFree(context);
            return nullptr;
        }

        if (!pwd_.empty())
        {
            auto* reply = static_cast<redisReply*>(redisCommand(context, "AUTH %s", pwd_.c_str()));
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
            {
                if (reply != nullptr)
                {
                    freeReplyObject(reply);
                }
                redisFree(context);
                return nullptr;
            }
            freeReplyObject(reply);
        }

        if (!keep_io_timeout && !SetIoTimeout(context, false))
        {
            redisFree(context);
            return nullptr;
        }

        return context;
    }

    static bool Ping(redisContext* context, bool restore_unbounded_timeout)
    {
        if (context == nullptr || (restore_unbounded_timeout && !SetIoTimeout(context, true)))
        {
            return false;
        }

        auto* reply = static_cast<redisReply*>(redisCommand(context, "PING"));
        const bool healthy = reply != nullptr && context->err == 0 && reply->type != REDIS_REPLY_ERROR;
        if (reply != nullptr)
        {
            freeReplyObject(reply);
        }
        if (!healthy)
        {
            return false;
        }
        return !restore_unbounded_timeout || SetIoTimeout(context, false);
    }

    bool CheckHealthConnection()
    {
        if (health_context_ == nullptr)
        {
            health_context_ = ConnectAuthenticated(true);
        }
        if (Ping(health_context_, false))
        {
            return true;
        }
        if (health_context_ != nullptr)
        {
            redisFree(health_context_);
            health_context_ = nullptr;
        }
        return false;
    }

    bool CheckOnePoolConnection(bool& checked)
    {
        checked = false;
        auto* context = getConNonBlock();
        if (context == nullptr)
        {
            return false;
        }
        checked = true;
        if (Ping(context, true))
        {
            returnConnection(context);
            return true;
        }

        returnBrokenConnection(context);
        return false;
    }

    void RepairOneConnection()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (connection_count_ >= poolSize_)
            {
                return;
            }
        }

        auto* context = ConnectAuthenticated(false);
        if (context == nullptr)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_.load(std::memory_order_acquire) || connection_count_ >= poolSize_)
        {
            redisFree(context);
            return;
        }
        connections_.push(context);
        ++connection_count_;
    }

    void CheckAndRepairConnections()
    {
        bool dependency_healthy = CheckHealthConnection();
        bool pool_connection_checked = false;
        const bool sampled_pool_healthy = CheckOnePoolConnection(pool_connection_checked);

        // If the reserved health connection cannot reconnect because Redis is
        // at its client limit, a verified business-pool connection still proves
        // that this process can serve Redis-backed requests.
        dependency_healthy = dependency_healthy || (pool_connection_checked && sampled_pool_healthy);
        if (!dependency_healthy)
        {
            healthy_.store(false, std::memory_order_release);
            cond_.notify_all();
            return;
        }

        RepairOneConnection();
        bool pool_usable = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pool_usable = connection_count_ > 0;
        }
        healthy_.store(pool_usable, std::memory_order_release);
        cond_.notify_all();
    }

    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    redisContext* health_context_ = nullptr;
    size_t connection_count_ = 0;
    std::mutex mutex_;
    std::condition_variable cond_;
    memochat::runtime::ExplicitThread check_thread_;
    bool ready_ = false;
    std::atomic<bool> healthy_{false};
    std::string startup_error_;
};

class RedisMgr
    : public Singleton<RedisMgr>
    , public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    ~RedisMgr();
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] bool Healthy() const noexcept;
    [[nodiscard]] const std::string& StartupError() const noexcept;
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool SetEx(const std::string& key, const std::string& value, int expire_seconds);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool HDel(const std::string& key, const std::string& field);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
    // SCARD: cardinality of a set; used for chat-server online-user load counts.
    bool SCard(const std::string& key, int& count);

    // Eval: run a Lua script atomically. Returns integer result or -1 on error.
    int64_t Eval(const std::string& script, const std::vector<std::string>& keys, const std::vector<std::string>& args);
    // EvalString: run a Lua script atomically and return its bulk string result.
    bool EvalString(const std::string& script,
                    const std::vector<std::string>& keys,
                    const std::vector<std::string>& args,
                    std::string& value);

    // Pipeline operations for reduced RTT
    // MGET: fetch multiple keys in one round-trip (returns map of key->value)
    // Keys with no value or error are omitted from result
    std::unordered_map<std::string, std::string> MGet(const std::vector<std::string>& keys);

    // MSET: set multiple key-value pairs in one round-trip
    bool MSet(const std::unordered_map<std::string, std::string>& kvs);

    // MPipeline: execute arbitrary commands in pipeline (advanced)
    // Returns vector of results in same order as commands
    // Caller must free each redisReply* in the returned vector
    std::vector<redisReply*> MPipeline(const std::vector<std::string>& commands);

    void Close()
    {
        _con_pool->Close();
        _con_pool->ClearConnections();
    }

    redisContext* getRawConnection()
    {
        return _con_pool->getConnection();
    }
    void returnConnection(redisContext* ctx)
    {
        _con_pool->returnConnection(ctx);
    }
    void returnBrokenConnection(redisContext* ctx)
    {
        _con_pool->returnBrokenConnection(ctx);
    }

    std::string acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout);

    bool releaseLock(const std::string& lockName, const std::string& identifier);

    void IncreaseCount(std::string server_name);
    void DecreaseCount(std::string server_name);
    void InitCount(std::string server_name);
    void DelCount(std::string server_name);

private:
    RedisMgr();
    std::unique_ptr<RedisConPool> _con_pool;
};
