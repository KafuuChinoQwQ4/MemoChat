#pragma once
#include "CallSessionTypes.hpp"
#include "MomentTypes.hpp"
#include "const.hpp"
#include "db/PqxxCompat.hpp"
#include "runtime/ExplicitThread.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdint>

class PooledSqlConnection
{
public:
    PooledSqlConnection(std::unique_ptr<pqxx::connection> con, int64_t lasttime)
        : _con(std::move(con))
        , _last_oper_time(lasttime)
    {
    }
    std::unique_ptr<pqxx::connection> _con;
    int64_t _last_oper_time;
};

class PostgresPool
{
public:
    PostgresPool(const std::string& connection_string, const std::string& schema, int poolSize)
        : connection_string_(connection_string)
        , schema_(schema.empty() ? "public" : schema)
        , poolSize_(poolSize)
        , b_stop_(false)
    {
        for (int i = 0; i < poolSize_; ++i)
        {
            auto con = CreateConnection();
            if (con)
            {
                pool_.push(std::move(con));
            }
        }

        if (pool_.empty())
        {
            startup_error_ = "postgres pool has no usable connections";
            return;
        }

        std::string thread_error;
        if (!_check_thread.Start(
                [this]()
                {
                    while (!b_stop_.load(std::memory_order_acquire))
                    {
                        checkConnection();
                        std::unique_lock<std::mutex> lock(mutex_);
                        cond_.wait_for(lock,
                                       std::chrono::seconds(60),
                                       [this]
                                       {
                                           return b_stop_.load(std::memory_order_acquire);
                                       });
                    }
                },
                &thread_error))
        {
            startup_error_ = "postgres pool checker init failed: " + thread_error;
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

    void checkConnection()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();

        auto currentTime = std::chrono::system_clock::now().time_since_epoch();

        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
        for (int i = 0; i < poolsize; i++)
        {
            auto con = std::move(pool_.front());
            pool_.pop();
            Defer defer(
                [this, &con]()
                {
                    pool_.push(std::move(con));
                });

            if (timestamp - con->_last_oper_time < 5)
            {
                continue;
            }

            {
                pqxx::read_transaction txn(*con->_con);
                const auto ping = txn.exec("SELECT 1");
                if (txn.ok() && ping.ok() && txn.commit())
                {
                    con->_last_oper_time = timestamp;
                    continue;
                }

                std::cout << "Error keeping connection alive: " << txn.error_message() << std::endl;
                txn.abort();
            }
            auto replacement = CreateConnection();
            if (replacement)
            {
                con = std::move(replacement);
            }
        }
    }

    std::unique_ptr<PooledSqlConnection> getConnection()
    {
        if (closed_.load(std::memory_order_acquire))
        {
            return nullptr;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait_for(lock,
                       std::chrono::seconds(2),
                       [this]
                       {
                           if (b_stop_)
                           {
                               return true;
                           }
                           return !pool_.empty();
                       });
        if (b_stop_ || closed_.load(std::memory_order_acquire))
        {
            return nullptr;
        }
        if (pool_.empty())
        {
            lock.unlock();
            return CreateConnection();
        }
        std::unique_ptr<PooledSqlConnection> con(std::move(pool_.front()));
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<PooledSqlConnection> con)
    {
        if (!con || !con->_con || !con->_con->is_open())
        {
            return;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_)
        {
            return;
        }
        pool_.push(std::move(con));
        cond_.notify_one();
    }

    void Close()
    {
        closed_.store(true, std::memory_order_release);
        b_stop_ = true;
        cond_.notify_all();
    }

    ~PostgresPool()
    {
        Close();
        std::string thread_error;
        if (_check_thread.Joinable() && !_check_thread.Join(&thread_error))
        {
            std::cout << "postgres pool checker join failed, error is " << thread_error << std::endl;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty())
        {
            pool_.pop();
        }
    }

private:
    std::string connection_string_;
    std::string schema_;
    int poolSize_;
    std::queue<std::unique_ptr<PooledSqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::atomic<bool> closed_{false};
    memochat::runtime::ExplicitThread _check_thread;
    bool ready_ = false;
    std::string startup_error_;

    std::unique_ptr<PooledSqlConnection> CreateConnection()
    {
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
        auto connection = std::make_unique<pqxx::connection>(connection_string_);
        if (!connection->is_open())
        {
            std::cout << "postgres connection init failed, error is " << connection->error_message() << std::endl;
            return nullptr;
        }
        pqxx::work txn(*connection);
        const std::string quoted_schema = txn.quote_name(schema_);
        const auto configured = txn.exec("SET search_path TO " + quoted_schema + ",public").no_rows();
        if (!txn.ok() || !configured.ok() || !txn.commit())
        {
            std::cout << "postgres connection init failed, error is " << txn.error_message() << std::endl;
            return nullptr;
        }
        return std::make_unique<PooledSqlConnection>(std::move(connection), timestamp);
    }
};

struct UserInfo
{
    std::string name;
    std::string pwd;
    int uid;
    std::string user_id;
    std::string email;
    std::string nick;
    std::string icon;
    std::string desc;
    int sex = 0;
};

struct R18AccessPolicyInfo
{
    int64_t adult_attested_at_ms = 0;
    int state = 0;
};

enum class RefreshTokenRotationStatus
{
    Success,
    Invalid,
    Expired,
    Reused,
    StorageError,
};

struct MediaAssetInfo
{
    int64_t media_id = 0;
    std::string media_key;
    int owner_uid = 0;
    std::string media_type;
    std::string origin_file_name;
    std::string mime;
    int64_t size_bytes = 0;
    std::string storage_provider;
    std::string storage_path;
    int64_t created_at_ms = 0;
    int64_t deleted_at_ms = 0;
    int status = 1;
};

class PostgresDao
{
public:
    PostgresDao();
    ~PostgresDao();
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] const std::string& StartupError() const noexcept;
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    int RegUserTransaction(const std::string& name,
                           const std::string& email,
                           const std::string& pwd,
                           const std::string& icon);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& email, const std::string& newpwd);
    bool IssueRefreshToken(int uid,
                           const std::string& selector,
                           const std::string& verifier_hash,
                           int ttl_seconds,
                           const std::string& user_agent,
                           const std::string& ip_hash);
    RefreshTokenRotationStatus RotateRefreshToken(const std::string& selector,
                                                  const std::string& verifier,
                                                  const std::string& replacement_selector,
                                                  const std::string& replacement_verifier_hash,
                                                  int ttl_seconds,
                                                  const std::string& user_agent,
                                                  const std::string& ip_hash,
                                                  int& uid);
    bool ResolveActiveRefreshTokenUserId(const std::string& selector, const std::string& verifier, int& uid);
    bool RevokeRefreshToken(const std::string& selector, const std::string& verifier, int& uid);
    bool RevokeAllRefreshTokensForUid(int uid);
    bool UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
    std::string GetUserPublicId(int uid);
    bool GetCallUserProfile(int uid, CallUserProfile& profile);
    bool IsFriend(int uid, int peer_uid);
    bool UpsertCallSession(const CallSessionInfo& session);
    bool GetCallSession(const std::string& call_id, CallSessionInfo& session);
    bool InsertMediaAsset(const MediaAssetInfo& asset);
    bool GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset);
    bool GrantMediaAccess(int64_t media_id, int grantee_uid, const std::string& scope, int64_t created_at_ms);
    bool GrantMediaGroupAccess(int64_t media_id,
                               int64_t group_id,
                               int owner_uid,
                               const std::string& scope,
                               int64_t created_at_ms);
    bool HasMediaAccess(int64_t media_id, int uid);
    bool GetUserInfo(int uid, UserInfo& user_info);
    bool GetR18AccessPolicy(int uid, R18AccessPolicyInfo& policy);
    bool AttestAdultForR18(int uid, int64_t attested_at_ms, R18AccessPolicyInfo& policy);
    bool TestProcedure(const std::string& email, int& uid, std::string& name);

    // Moments operations
    bool AddMoment(const MomentInfo& moment, int64_t* moment_id = nullptr);
    bool GetMomentsFeed(int viewer_uid,
                        int64_t last_moment_id,
                        int limit,
                        int author_uid,
                        std::vector<MomentInfo>& moments,
                        bool& has_more);
    bool GetMomentsFeedCandidates(int viewer_uid,
                                  int64_t last_moment_id,
                                  int limit,
                                  int author_uid,
                                  std::vector<MomentInfo>& moments,
                                  bool& has_more);
    bool CanViewMoment(int viewer_uid, const MomentInfo& moment);
    bool GetMomentById(int64_t moment_id, MomentInfo& moment);
    bool DeleteMoment(int64_t moment_id, int uid);
    bool AddMomentLike(int64_t moment_id, int uid);
    bool RemoveMomentLike(int64_t moment_id, int uid);
    bool HasLikedMoment(int64_t moment_id, int uid);
    bool GetMomentLikes(int64_t moment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more);
    bool AddMomentComment(const MomentCommentInfo& comment);
    bool DeleteMomentComment(int64_t comment_id, int uid);
    bool GetMomentCommentById(int64_t comment_id, MomentCommentInfo& comment);
    bool GetMomentComments(int64_t moment_id,
                           int64_t last_comment_id,
                           int limit,
                           std::vector<MomentCommentInfo>& comments,
                           bool& has_more);
    bool AddMomentCommentLike(int64_t comment_id, int uid);
    bool RemoveMomentCommentLike(int64_t comment_id, int uid);
    bool HasLikedMomentComment(int64_t comment_id, int uid);
    bool GetMomentCommentLikes(int64_t comment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more);

private:
    void WarmupAuthQueries();
    std::string GenerateUserPublicId();
    std::unique_ptr<PostgresPool> pool_;
    // Optional dedicated connection to the account database (memo_account) for
    // cross-domain user-info reads. Set from [AccountPostgres] when present;
    // empty means "use the pooled connection" (monolith / account-owning
    // services where the user table lives in the primary DB).
    std::string account_connection_string_;
    bool ready_ = false;
    std::string startup_error_;
};
