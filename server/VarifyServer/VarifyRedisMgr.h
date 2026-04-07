#pragma once

#include <hiredis/hiredis.h>
#include <queue>
#include <atomic>
#include <mutex>
#include <string>
#include <memory>

namespace varifyservice {

class VarifyRedisConPool {
public:
    VarifyRedisConPool(size_t poolSize, const char* host, int port, const char* pwd);
    ~VarifyRedisConPool();

    redisContext* GetConnection();
    void ReturnConnection(redisContext* context);
    void Close();

private:
    void CheckThreadProc();

    std::atomic<bool> b_stop_{false};
    size_t poolSize_;
    const char* host_;
    const char* pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread check_thread_;
};

class VarifyRedisMgr {
public:
    static VarifyRedisMgr& Instance();

    bool Get(const std::string& key, std::string& value);
    bool SetEx(const std::string& key, const std::string& value, int expire_sec);
    int64_t IncrWithExpire(const std::string& key, int expire_sec);
    int64_t TTL(const std::string& key);
    bool Del(const std::string& key);
    void Close();

    bool Ping();

private:
    VarifyRedisMgr();
    ~VarifyRedisMgr();

    std::unique_ptr<VarifyRedisConPool> pool_;
};

} // namespace varifyservice
