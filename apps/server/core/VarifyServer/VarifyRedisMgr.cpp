#include "VarifyRedisMgr.hpp"
#include "ConfigMgr.hpp"
#include "common_lua_scripts.hpp"

#include <charconv>
#include <iostream>
#include <cstring>

import memochat.varify.redis_algorithms;

namespace varifyservice
{
namespace redis_modules = memochat::varify::redis::modules;

VarifyRedisConPool::VarifyRedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
    : poolSize_(poolSize)
    , host_(host == nullptr ? "" : host)
    , port_(port)
    , pwd_(pwd == nullptr ? "" : pwd)
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

        if (!pwd_.empty() && redis_modules::HasPassword(pwd_[0]))
        {
            auto reply = (redisReply*) redisCommand(context, "AUTH %s", pwd_.c_str());
            if (!redis_modules::HasReply(reply != nullptr) ||
                redis_modules::IsExpectedReplyType(reply->type, REDIS_REPLY_ERROR))
            {
                if (reply)
                    freeReplyObject(reply);
                redisFree(context);
                continue;
            }
            freeReplyObject(reply);
        }

        connections_.push(context);
    }

    if (connections_.empty())
    {
        startup_error_ = "Varify Redis pool has no usable connections";
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
                                           std::chrono::seconds(60),
                                           [this]
                                           {
                                               return b_stop_.load(std::memory_order_acquire);
                                           }))
                        {
                            break;
                        }
                    }
                    CheckThreadProc();
                }
            },
            &thread_error))
    {
        startup_error_ = "Varify Redis pool checker init failed: " + thread_error;
        return;
    }
    ready_ = true;
}

VarifyRedisConPool::~VarifyRedisConPool()
{
    Close();
}

redisContext* VarifyRedisConPool::GetConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,
               [this]()
               {
                   return b_stop_ || !ready_ || !connections_.empty();
               });
    if (b_stop_ || !ready_)
        return nullptr;
    auto* context = connections_.front();
    connections_.pop();
    return context;
}

bool VarifyRedisConPool::Ready() const noexcept
{
    return ready_;
}

const std::string& VarifyRedisConPool::StartupError() const noexcept
{
    return startup_error_;
}

void VarifyRedisConPool::ReturnConnection(redisContext* context)
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

void VarifyRedisConPool::Close()
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
            std::cerr << "varify redis pool checker join failed: " << thread_error << std::endl;
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty())
    {
        redisFree(connections_.front());
        connections_.pop();
    }
}

void VarifyRedisConPool::CheckThreadProc()
{
    std::lock_guard<std::mutex> lock(mutex_);
    size_t pool_size = connections_.size();
    for (size_t i = 0; i < pool_size; ++i)
    {
        auto* context = connections_.front();
        connections_.pop();

        auto reply = (redisReply*) redisCommand(context, "PING");
        if (redis_modules::ShouldDropPoolConnection(reply != nullptr, context->err))
        {
            if (reply)
                freeReplyObject(reply);
            redisFree(context);
            continue;
        }
        freeReplyObject(reply);
        connections_.push(context);
    }
}

VarifyRedisMgr::VarifyRedisMgr()
{
    auto& cfg = ConfigMgr::Inst();
    auto host = cfg["Redis"]["Host"];
    auto port_str = cfg["Redis"]["Port"];
    auto pwd = cfg["Redis"]["Passwd"];
    int port = redis_modules::DefaultRedisPort();
    if (!port_str.empty())
    {
        int configured_port = 0;
        const auto [ptr, ec] = std::from_chars(port_str.data(), port_str.data() + port_str.size(), configured_port);
        if (ec == std::errc{} && ptr == port_str.data() + port_str.size() && configured_port > 0 &&
            configured_port <= 65535)
        {
            port = configured_port;
        }
    }
    pool_.reset(new VarifyRedisConPool(redis_modules::DefaultConnectionPoolSize(), host.c_str(), port, pwd.c_str()));
}

VarifyRedisMgr::~VarifyRedisMgr()
{
    Close();
}

VarifyRedisMgr& VarifyRedisMgr::Instance()
{
    static VarifyRedisMgr mgr;
    return mgr;
}

bool VarifyRedisMgr::Ready() const noexcept
{
    return pool_ && pool_->Ready();
}

const std::string& VarifyRedisMgr::StartupError() const noexcept
{
    static const std::string allocation_error = "Varify Redis pool allocation failed";
    return pool_ ? pool_->StartupError() : allocation_error;
}

redisContext* GetContext(VarifyRedisConPool* pool)
{
    return pool->GetConnection();
}

redisContext* GetCtx(VarifyRedisConPool* p)
{
    return p->GetConnection();
}

bool VarifyRedisMgr::Get(const std::string& key, std::string& value)
{
    auto* ctx = pool_->GetConnection();
    if (!ctx)
        return false;
    auto reply = (redisReply*) redisCommand(ctx, "GET %s", key.c_str());
    bool ok = false;
    if (reply && redis_modules::IsExpectedReplyType(reply->type, REDIS_REPLY_STRING))
    {
        value = reply->str;
        ok = true;
    }
    if (reply)
        freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

bool VarifyRedisMgr::SetEx(const std::string& key, const std::string& value, int expire_sec)
{
    auto* ctx = pool_->GetConnection();
    if (!ctx)
        return false;
    auto reply = (redisReply*) redisCommand(ctx, "SETEX %s %d %s", key.c_str(), expire_sec, value.c_str());
    bool ok = false;
    if (reply && redis_modules::IsExpectedReplyType(reply->type, REDIS_REPLY_STATUS))
    {
        ok = (strcmp(reply->str, "OK") == 0);
    }
    if (reply)
        freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

int64_t VarifyRedisMgr::IncrWithExpire(const std::string& key, int expire_sec)
{
    auto* ctx = pool_->GetConnection();
    if (!ctx)
        return redis_modules::RedisCounterError();

    const std::string lua_script(memochat::common::lua_scripts::kincr_with_expire);

    auto reply = (redisReply*) redisCommand(ctx, "EVAL %s 1 %s %d", lua_script.c_str(), key.c_str(), expire_sec);
    int64_t count = redis_modules::RedisCounterError();
    if (reply && redis_modules::IsExpectedReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        count = reply->integer;
    }
    if (reply)
        freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return count;
}

int64_t VarifyRedisMgr::TTL(const std::string& key)
{
    auto* ctx = pool_->GetConnection();
    if (!ctx)
        return redis_modules::RedisMissingTtl();
    auto reply = (redisReply*) redisCommand(ctx, "TTL %s", key.c_str());
    int64_t ttl = redis_modules::RedisMissingTtl();
    if (reply && redis_modules::IsExpectedReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        ttl = reply->integer;
    }
    if (reply)
        freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ttl;
}

bool VarifyRedisMgr::Del(const std::string& key)
{
    auto* ctx = pool_->GetConnection();
    if (!ctx)
        return false;
    auto reply = (redisReply*) redisCommand(ctx, "DEL %s", key.c_str());
    bool ok = false;
    if (reply && redis_modules::IsExpectedReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        ok = true;
    }
    if (reply)
        freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

bool VarifyRedisMgr::Ping()
{
    auto* ctx = pool_->GetConnection();
    if (!ctx)
        return false;
    auto reply = (redisReply*) redisCommand(ctx, "PING");
    bool ok = false;
    if (reply && redis_modules::IsExpectedReplyType(reply->type, REDIS_REPLY_STATUS))
    {
        ok = true;
    }
    if (reply)
        freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

void VarifyRedisMgr::Close()
{
    pool_->Close();
}

} // namespace varifyservice
