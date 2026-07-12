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
        : poolSize_(poolSize)
        , host_(host == nullptr ? "" : host)
        , port_(port)
        , b_stop_(false)
        , pwd_(pwd == nullptr ? "" : pwd)
        , counter_(0)
        , fail_count_(0)
    {
        for (size_t i = 0; i < poolSize_; ++i)
        {
            auto* context = redisConnect(host, port);
            if (context == nullptr || context->err != 0)
            {
                if (context != nullptr)
                {
                    redisFree(context);
                }
                continue;
            }

            auto reply = (redisReply*) redisCommand(context, "AUTH %s", pwd_.c_str());
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
            {
                std::cout << "redis auth failed" << std::endl;
                if (reply != nullptr)
                {
                    freeReplyObject(reply);
                }
                redisFree(context);
                continue;
            }

            freeReplyObject(reply);
            std::cout << "redis auth success" << std::endl;
            connections_.push(context);
        }

        if (connections_.empty())
        {
            startup_error_ = "redis pool has no usable connections";
            return;
        }

        std::string thread_error;
        if (!check_thread_.Start(
                [this]()
                {
                    while (!b_stop_.load(std::memory_order_acquire))
                    {
                        {
                            std::unique_lock<std::mutex> lock(mutex_);
                            if (cond_.wait_for(lock,
                                               std::chrono::seconds(1),
                                               [this]
                                               {
                                                   return b_stop_.load(std::memory_order_acquire);
                                               }))
                            {
                                break;
                            }
                        }
                        counter_++;
                        if (counter_ >= 60)
                        {
                            checkThreadPro();
                            counter_ = 0;
                        }
                    }
                },
                &thread_error))
        {
            startup_error_ = "redis pool checker init failed: " + thread_error;
            return;
        }
        ready_ = true;
    }

    [[nodiscard]] bool Ready() const noexcept
    {
        return ready_;
    }

    [[nodiscard]] const std::string& StartupError() const noexcept
    {
        return startup_error_;
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
    }

    redisContext* getConnection()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock,
                   [this]
                   {
                       if (b_stop_ || !ready_)
                       {
                           return true;
                       }
                       return !connections_.empty();
                   });

        if (b_stop_ || !ready_)
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
        if (b_stop_ || !ready_)
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
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_)
        {
            redisFree(context);
            return;
        }
        connections_.push(context);
        cond_.notify_one();
    }

    void returnBrokenConnection(redisContext* context)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_)
        {
            redisFree(context);
            return;
        }
        redisFree(context);
        --poolSize_;
        cond_.notify_one();
    }

    void Close()
    {
        if (b_stop_.exchange(true))
        {
            return;
        }
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
    bool reconnect()
    {
        auto context = redisConnect(host_.c_str(), port_);
        if (context == nullptr || context->err != 0)
        {
            if (context != nullptr)
            {
                redisFree(context);
            }
            return false;
        }

        auto reply = (redisReply*) redisCommand(context, "AUTH %s", pwd_.c_str());
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
        {
            std::cout << "redis auth failed" << std::endl;

            if (reply != nullptr)
            {
                freeReplyObject(reply);
            }
            redisFree(context);
            return false;
        }

        freeReplyObject(reply);
        std::cout << "redis auth success" << std::endl;
        returnConnection(context);
        return true;
    }

    void checkThreadPro()
    {
        size_t pool_size;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pool_size = connections_.size();
        }

        for (int i = 0; i < pool_size && !b_stop_; ++i)
        {
            auto* context = getConNonBlock();
            if (context == nullptr)
            {
                break;
            }

            redisReply* reply = (redisReply*) redisCommand(context, "PING");

            if (context->err)
            {
                std::cout << "Connection error: " << context->err << std::endl;
                if (reply)
                {
                    freeReplyObject(reply);
                }
                redisFree(context);
                fail_count_++;
                continue;
            }

            if (!reply || reply->type == REDIS_REPLY_ERROR)
            {
                std::cout << "reply is null, redis ping failed: " << std::endl;
                if (reply)
                {
                    freeReplyObject(reply);
                }
                redisFree(context);
                fail_count_++;
                continue;
            }

            freeReplyObject(reply);
            returnConnection(context);
        }

        while (fail_count_ > 0)
        {
            auto res = reconnect();
            if (res)
            {
                fail_count_--;
            }
            else
            {
                break;
            }
        }
    }

    void checkThread()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_)
        {
            return;
        }
        auto pool_size = connections_.size();
        for (int i = 0; i < pool_size && !b_stop_; i++)
        {
            auto* context = connections_.front();
            connections_.pop();
            auto reply = (redisReply*) redisCommand(context, "PING");
            if (reply != nullptr && context->err == 0 && reply->type != REDIS_REPLY_ERROR)
            {
                freeReplyObject(reply);
                connections_.push(context);
                continue;
            }
            if (reply != nullptr)
            {
                freeReplyObject(reply);
            }
            std::cout << "Error keeping Redis connection alive" << std::endl;
            redisFree(context);
            context = redisConnect(host_.c_str(), port_);
            if (context == nullptr || context->err != 0)
            {
                if (context != nullptr)
                {
                    redisFree(context);
                }
                continue;
            }

            reply = (redisReply*) redisCommand(context, "AUTH %s", pwd_.c_str());
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
            {
                std::cout << "redis auth failed" << std::endl;
                if (reply != nullptr)
                {
                    freeReplyObject(reply);
                }
                redisFree(context);
                continue;
            }

            freeReplyObject(reply);
            std::cout << "redis auth success" << std::endl;
            connections_.push(context);
        }
    }
    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    std::atomic<int> fail_count_;
    std::mutex mutex_;
    std::condition_variable cond_;
    memochat::runtime::ExplicitThread check_thread_;
    int counter_;
    bool ready_ = false;
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
