#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisConPool::RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
    : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
    for (size_t i = 0; i < poolSize_; ++i) {
        auto* context = redisConnect(host, port);
        if (context == nullptr || context->err != 0) {
            if (context != nullptr) {
                redisFree(context);
            }
            continue;
        }

        auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
        if (reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Redis Auth failed" << std::endl;
            freeReplyObject(reply);
            continue;
        }

        freeReplyObject(reply);
        std::cout << "Redis Auth success" << std::endl;
        connections_.push(context);
    }
}

RedisConPool::~RedisConPool() {
    Close();
}

void RedisConPool::Close() {
    b_stop_ = true;
    cond_.notify_all();
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        auto* context = connections_.front();
        connections_.pop();
        if(context) redisFree(context);
    }
}

redisContext* RedisConPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] {
        if (b_stop_) return true;
        return !connections_.empty();
        });
    if (b_stop_) return nullptr;
    auto* context = connections_.front();
    connections_.pop();
    return context;
}

void RedisConPool::returnConnection(redisContext* context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_) {
        if(context) redisFree(context);
        return;
    }
    connections_.push(context);
    cond_.notify_one();
}

RedisMgr::RedisMgr() {
    auto host = ConfigMgr::Inst()["Redis"]["Host"];
    auto port = ConfigMgr::Inst()["Redis"]["Port"];
    auto pwd = ConfigMgr::Inst()["Redis"]["Passwd"];
    _con_pool.reset(new RedisConPool(5, host.c_str(), std::atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr() {
    Close();
}

void RedisMgr::Close() {
    _con_pool->Close();
}

bool RedisMgr::Get(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
    if (reply == NULL) {
        _con_pool->returnConnection(connect);
        return false;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
    if (NULL == reply) {
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0))) {
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

// 占位实现
bool RedisMgr::Auth(const std::string& password) { return true; }
bool RedisMgr::LPush(const std::string& key, const std::string& value) { return false; }
bool RedisMgr::LPop(const std::string& key, std::string& value) { return false; }
bool RedisMgr::RPush(const std::string& key, const std::string& value) { return false; }
bool RedisMgr::RPop(const std::string& key, std::string& value) { return false; }
bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) { return false; }
bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen) { return false; }
bool RedisMgr::HDel(const std::string& key, const std::string& field) {
    if (_con == nullptr) return false;
    
    // 使用 redisCommand 执行 HDEL 命令
    redisReply* reply = (redisReply*)redisCommand(_con, "HDEL %s %s", key.c_str(), field.c_str());
    
    if (reply == nullptr) {
        std::cerr << "HDel command failed" << std::endl;
        return false;
    }
    
    // HDEL 返回删除的个数，>=0 即为成功（即使不存在返回0也算执行成功）
    bool success = false;
    if (reply->type == REDIS_REPLY_INTEGER) {
        success = true;
    }
    
    freeReplyObject(reply);
    return success;
}
std::string RedisMgr::HGet(const std::string& key, const std::string& hkey) { return ""; }
bool RedisMgr::Del(const std::string& key) { return false; }
bool RedisMgr::ExistsKey(const std::string& key) { return false; }