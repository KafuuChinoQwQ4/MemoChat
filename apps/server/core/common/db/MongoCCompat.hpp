#pragma once

#include <mongoc/mongoc.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace memo::db
{
class MongoRuntime
{
public:
    MongoRuntime() noexcept
    {
        mongoc_init();
    }

    MongoRuntime(const MongoRuntime&) = delete;
    MongoRuntime& operator=(const MongoRuntime&) = delete;

    ~MongoRuntime()
    {
        mongoc_cleanup();
    }
};

inline void EnsureMongoRuntimeInitialized() noexcept
{
    static MongoRuntime runtime;
    (void) runtime;
}

inline std::string MongoErrorMessage(const bson_error_t& error)
{
    return error.message;
}

class MongoClientPool;

class MongoClientLease
{
public:
    MongoClientLease() = default;
    MongoClientLease(const MongoClientLease&) = delete;
    MongoClientLease& operator=(const MongoClientLease&) = delete;

    MongoClientLease(MongoClientLease&& other) noexcept
        : owner_(std::exchange(other.owner_, nullptr))
        , client_(std::exchange(other.client_, nullptr))
    {
    }

    MongoClientLease& operator=(MongoClientLease&& other) noexcept;

    ~MongoClientLease();

    mongoc_client_t* get() const noexcept
    {
        return client_;
    }

    explicit operator bool() const noexcept
    {
        return client_ != nullptr;
    }

private:
    friend class MongoClientPool;

    MongoClientLease(MongoClientPool* owner, mongoc_client_t* client) noexcept
        : owner_(owner)
        , client_(client)
    {
    }

    void Reset() noexcept;

    MongoClientPool* owner_ = nullptr;
    mongoc_client_t* client_ = nullptr;
};

class MongoClientPool
{
public:
    MongoClientPool() = default;
    MongoClientPool(const MongoClientPool&) = delete;
    MongoClientPool& operator=(const MongoClientPool&) = delete;

    ~MongoClientPool()
    {
        Reset();
    }

    bool Open(std::string_view uri, std::string& error)
    {
        Reset();
        error.clear();
        EnsureMongoRuntimeInitialized();

        const std::string owned_uri(uri);
        bson_error_t driver_error{};
        uri_ = mongoc_uri_new_with_error(owned_uri.c_str(), &driver_error);
        if (uri_ == nullptr)
        {
            error = MongoErrorMessage(driver_error);
            return false;
        }

        pool_ = mongoc_client_pool_new_with_error(uri_, &driver_error);
        if (pool_ == nullptr)
        {
            error = MongoErrorMessage(driver_error);
            mongoc_uri_destroy(uri_);
            uri_ = nullptr;
            return false;
        }
        return true;
    }

    bool IsOpen() const noexcept
    {
        return pool_ != nullptr;
    }

    MongoClientLease Acquire() noexcept
    {
        return pool_ == nullptr ? MongoClientLease{} : MongoClientLease(this, mongoc_client_pool_pop(pool_));
    }

private:
    friend class MongoClientLease;

    void Release(mongoc_client_t* client) noexcept
    {
        if (client == nullptr)
        {
            return;
        }
        if (pool_ == nullptr)
        {
            mongoc_client_destroy(client);
            return;
        }
        mongoc_client_pool_push(pool_, client);
    }

    void Reset() noexcept
    {
        if (pool_ != nullptr)
        {
            mongoc_client_pool_destroy(pool_);
            pool_ = nullptr;
        }
        if (uri_ != nullptr)
        {
            mongoc_uri_destroy(uri_);
            uri_ = nullptr;
        }
    }

    mongoc_uri_t* uri_ = nullptr;
    mongoc_client_pool_t* pool_ = nullptr;
};

inline MongoClientLease& MongoClientLease::operator=(MongoClientLease&& other) noexcept
{
    if (this != &other)
    {
        Reset();
        owner_ = std::exchange(other.owner_, nullptr);
        client_ = std::exchange(other.client_, nullptr);
    }
    return *this;
}

inline MongoClientLease::~MongoClientLease()
{
    Reset();
}

inline void MongoClientLease::Reset() noexcept
{
    if (owner_ != nullptr && client_ != nullptr)
    {
        owner_->Release(client_);
    }
    owner_ = nullptr;
    client_ = nullptr;
}
} // namespace memo::db
