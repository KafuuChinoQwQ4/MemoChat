#pragma once
#include "const.hpp"
#include "runtime/ExplicitThread.hpp"
#include <hiredis/hiredis.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "Singleton.hpp"
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
            auto* context = redisConnect(host_.c_str(), port);
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
                std::cout << "redis auth status" << std::endl;

                if (reply != nullptr)
                {
                    freeReplyObject(reply);
                }
                redisFree(context);
                continue;
            }

            freeReplyObject(reply);
            std::cout << "redis auth status" << std::endl;
            connections_.push(context);
        }

        if (!check_thread_.Start(
                [this]() noexcept
                {
                    while (!b_stop_)
                    {
                        counter_++;
                        if (counter_ >= 60)
                        {
                            checkThreadPro();
                            counter_ = 0;
                        }

                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                },
                &startup_error_))
        {
            b_stop_ = true;
            std::cerr << "redis health thread start failed: " << startup_error_ << std::endl;
        }
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
                       if (b_stop_)
                       {
                           return true;
                       }
                       return !connections_.empty();
                   });

        if (b_stop_)
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
        if (b_stop_)
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
            return;
        }
        connections_.push(context);
        cond_.notify_one();
    }

    void Close()
    {
        b_stop_ = true;
        cond_.notify_all();
        if (check_thread_.Joinable())
        {
            std::string error;
            if (!check_thread_.Join(&error))
            {
                std::cerr << "redis health thread join failed: " << error << std::endl;
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
            std::cout << "redis auth status" << std::endl;

            if (reply != nullptr)
            {
                freeReplyObject(reply);
            }
            redisFree(context);
            return false;
        }

        freeReplyObject(reply);
        std::cout << "redis auth status" << std::endl;
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
            redisContext* ctx = nullptr;

            bool bsuccess = false;
            auto* context = getConNonBlock();
            if (context == nullptr)
            {
                break;
            }

            redisReply* reply = nullptr;

            reply = (redisReply*) redisCommand(context, "PING");

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
                std::cout << "redis auth status" << std::endl;
                if (reply)
                {
                    freeReplyObject(reply);
                }
                redisFree(context);
                fail_count_++;
                continue;
            }

            // std::cout << "redis auth status" << std::endl;
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
            if (!reply)
            {
                std::cout << "redis auth status" << std::endl;
                connections_.push(context);
                continue;
            }
            freeReplyObject(reply);
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
    std::string startup_error_;
    int counter_;
};

class RedisMgr
    : public Singleton<RedisMgr>
    , public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;

public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool SetEx(const std::string& key, const std::string& value, int expire_seconds);
    bool SetNxEx(const std::string& key, const std::string& value, int expire_seconds);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool HDel(const std::string& key, const std::string& field);
    bool SAdd(const std::string& key, const std::string& member);
    bool SRem(const std::string& key, const std::string& member);
    bool SMembers(const std::string& key, std::vector<std::string>& members);
    bool Keys(const std::string& pattern, std::vector<std::string>& keys);
    bool Incr(const std::string& key, int64_t& value);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
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
