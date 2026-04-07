#include "VarifyRedisMgr.h"
#include "ConfigMgr.h"

#include <iostream>
#include <cstdlib>

namespace varifyservice {

VarifyRedisConPool::VarifyRedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
    : poolSize_(poolSize), host_(host), port_(port), pwd_(pwd) {

    for (size_t i = 0; i < poolSize_; ++i) {
        auto* context = redisConnect(host, port);
        if (context == nullptr || context->err != 0) {
            if (context != nullptr) {
                redisFree(context);
            }
            continue;
        }

        if (pwd && pwd[0] != '\0') {
            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
            if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
                if (reply) freeReplyObject(reply);
                redisFree(context);
                continue;
            }
            freeReplyObject(reply);
        }

        connections_.push(context);
    }

    check_thread_ = std::thread([this]() {
        while (!b_stop_) {
            std::this_thread::sleep_for(std::chrono::seconds(60));
            if (b_stop_) break;
            CheckThreadProc();
        }
    });
}

VarifyRedisConPool::~VarifyRedisConPool() {
    Close();
}

redisContext* VarifyRedisConPool::GetConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return b_stop_ || !connections_.empty(); });
    if (b_stop_) return nullptr;
    auto* context = connections_.front();
    connections_.pop();
    return context;
}

void VarifyRedisConPool::ReturnConnection(redisContext* context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_) return;
    connections_.push(context);
    cond_.notify_one();
}

void VarifyRedisConPool::Close() {
    b_stop_ = true;
    cond_.notify_all();
    if (check_thread_.joinable()) {
        check_thread_.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        redisFree(connections_.front());
        connections_.pop();
    }
}

void VarifyRedisConPool::CheckThreadProc() {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t pool_size = connections_.size();
    for (size_t i = 0; i < pool_size; ++i) {
        auto* context = connections_.front();
        connections_.pop();

        auto reply = (redisReply*)redisCommand(context, "PING");
        if (reply == nullptr || context->err != 0) {
            if (reply) freeReplyObject(reply);
            redisFree(context);
            continue;
        }
        freeReplyObject(reply);
        connections_.push(context);
    }
}

VarifyRedisMgr::VarifyRedisMgr() {
    auto& cfg = ConfigMgr::Inst();
    auto host = cfg["Redis"]["Host"];
    auto port_str = cfg["Redis"]["Port"];
    auto pwd = cfg["Redis"]["Passwd"];
    int port = 6379;
    if (!port_str.empty()) {
        port = std::atoi(port_str.c_str());
    }
    pool_.reset(new VarifyRedisConPool(5, host.c_str(), port, pwd.c_str()));
}

VarifyRedisMgr::~VarifyRedisMgr() {
    Close();
}

VarifyRedisMgr& VarifyRedisMgr::Instance() {
    static VarifyRedisMgr mgr;
    return mgr;
}

redisContext* GetContext(VarifyRedisConPool* pool) {
    return pool->GetConnection();
}

redisContext* GetCtx(VarifyRedisConPool* p) { return p->GetConnection(); }

bool VarifyRedisMgr::Get(const std::string& key, std::string& value) {
    auto* ctx = pool_->GetConnection();
    if (!ctx) return false;
    auto reply = (redisReply*)redisCommand(ctx, "GET %s", key.c_str());
    bool ok = false;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        value = reply->str;
        ok = true;
    }
    if (reply) freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

bool VarifyRedisMgr::SetEx(const std::string& key, const std::string& value, int expire_sec) {
    auto* ctx = pool_->GetConnection();
    if (!ctx) return false;
    auto reply = (redisReply*)redisCommand(ctx, "SETEX %s %d %s", key.c_str(), expire_sec, value.c_str());
    bool ok = false;
    if (reply && (reply->type == REDIS_REPLY_STATUS)) {
        ok = (strcmp(reply->str, "OK") == 0);
    }
    if (reply) freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

int64_t VarifyRedisMgr::IncrWithExpire(const std::string& key, int expire_sec) {
    auto* ctx = pool_->GetConnection();
    if (!ctx) return -1;
    auto reply = (redisReply*)redisCommand(ctx, "INCR %s", key.c_str());
    int64_t count = -1;
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        count = reply->integer;
        if (count == 1) {
            freeReplyObject(reply);
            reply = (redisReply*)redisCommand(ctx, "EXPIRE %s %d", key.c_str(), expire_sec);
            if (reply && reply->type == REDIS_REPLY_INTEGER) {
                // TTL set
            }
        }
    }
    if (reply) freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return count;
}

int64_t VarifyRedisMgr::TTL(const std::string& key) {
    auto* ctx = pool_->GetConnection();
    if (!ctx) return -2;
    auto reply = (redisReply*)redisCommand(ctx, "TTL %s", key.c_str());
    int64_t ttl = -2;
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        ttl = reply->integer;
    }
    if (reply) freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ttl;
}

bool VarifyRedisMgr::Del(const std::string& key) {
    auto* ctx = pool_->GetConnection();
    if (!ctx) return false;
    auto reply = (redisReply*)redisCommand(ctx, "DEL %s", key.c_str());
    bool ok = false;
    if (reply && reply->type == REDIS_REPLY_INTEGER) {
        ok = true;
    }
    if (reply) freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

bool VarifyRedisMgr::Ping() {
    auto* ctx = pool_->GetConnection();
    if (!ctx) return false;
    auto reply = (redisReply*)redisCommand(ctx, "PING");
    bool ok = false;
    if (reply && reply->type == REDIS_REPLY_STATUS) {
        ok = true;
    }
    if (reply) freeReplyObject(reply);
    pool_->ReturnConnection(ctx);
    return ok;
}

void VarifyRedisMgr::Close() {
    pool_->Close();
}

} // namespace varifyservice
