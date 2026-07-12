#include "PostgresDao.hpp"
#include "ConfigMgr.hpp"
#include "db/PqxxCompat.hpp"
#include <charconv>
#include <unordered_map>

import memochat.gate.postgres_dao_algorithms;

namespace postgres_dao_modules = memochat::gate::postgres_dao::modules;

namespace
{
template <typename Transaction> bool TransactionOk(const char* operation, const Transaction& txn)
{
    if (txn.ok())
    {
        return true;
    }
    std::cerr << operation << " PostgreSQL error: " << txn.error_message() << std::endl;
    return false;
}

// Batch-fetch (nick, icon) for a set of uids from the account database.
// Returns empty map on error or empty input.
std::unordered_map<int, std::pair<std::string, std::string>> FetchUserProfiles(const std::string& conn_str,
                                                                               const std::vector<int>& uids)
{
    std::unordered_map<int, std::pair<std::string, std::string>> result;
    if (!postgres_dao_modules::ShouldFetchUserProfiles(conn_str.empty(), uids.empty()))
        return result;
    std::string uid_list;
    for (size_t i = 0; i < uids.size(); ++i)
    {
        if (i > 0)
            uid_list += ',';
        uid_list += std::to_string(uids[i]);
    }
    pqxx::connection con(conn_str);
    if (!con.is_open())
    {
        return result;
    }
    pqxx::read_transaction txn(con);
    const auto rows = txn.exec("SELECT uid, COALESCE(nick,'') AS nick, COALESCE(icon,'') AS icon "
                               "FROM \"user\" WHERE uid IN (" +
                               uid_list + ")");
    if (!TransactionOk("FetchUserProfiles", txn))
    {
        return result;
    }
    for (const auto& row : rows)
        result[row["uid"].as<int>()] = {row["nick"].c_str(), row["icon"].c_str()};
    if (!TransactionOk("FetchUserProfiles", txn))
    {
        result.clear();
    }
    return result;
}

std::string BuildConnectionString()
{
    auto& cfg = ConfigMgr::Inst();
    const auto host = cfg["Postgres"]["Host"];
    const auto port = cfg["Postgres"]["Port"];
    const auto pwd = cfg["Postgres"]["Passwd"];
    const auto database = cfg["Postgres"]["Database"];
    const auto schema = cfg["Postgres"]["Schema"];
    const auto user = cfg["Postgres"]["User"];
    const auto sslmode = cfg["Postgres"]["SslMode"];
    std::string conn = "host=" + host + " port=" + port + " user=" + user + " password=" + pwd + " dbname=" + database +
                       " sslmode=" + (sslmode.empty() ? postgres_dao_modules::DefaultSslMode() : sslmode) +
                       " connect_timeout=" + std::to_string(postgres_dao_modules::ConnectTimeoutSeconds());
    return conn;
}

// Builds a connection string for cross-domain account reads. Returns "" when
// the section is absent so callers fall back to the pooled connection. After
// the Phase 2 DB split, services that don't own the user table (e.g. moments)
// set [AccountPostgres] -> memo_account so GetUserInfo reads the authoritative
// user store instead of their own database (where "user" doesn't exist).
std::string BuildAccountConnectionString()
{
    auto& cfg = ConfigMgr::Inst();
    if (!postgres_dao_modules::ShouldUseAccountPostgresSection(cfg["AccountPostgres"]["Host"].empty()))
    {
        return "";
    }
    const auto host = cfg["AccountPostgres"]["Host"];
    const auto port = cfg["AccountPostgres"]["Port"];
    const auto pwd = cfg["AccountPostgres"]["Passwd"];
    const auto database = cfg["AccountPostgres"]["Database"];
    const auto schema = cfg["AccountPostgres"]["Schema"];
    const auto user = cfg["AccountPostgres"]["User"];
    const auto sslmode = cfg["AccountPostgres"]["SslMode"];
    std::string conn = "host=" + host + " port=" + port + " user=" + user + " password=" + pwd + " dbname=" + database +
                       " sslmode=" + (sslmode.empty() ? postgres_dao_modules::DefaultSslMode() : sslmode) +
                       " options=-csearch_path=" + (schema.empty() ? postgres_dao_modules::DefaultSchema() : schema) +
                       "," + postgres_dao_modules::DefaultSchema() +
                       " connect_timeout=" + std::to_string(postgres_dao_modules::ConnectTimeoutSeconds());
    return conn;
}

std::string PostgresSchemaName()
{
    auto& cfg = ConfigMgr::Inst();
    const auto schema = cfg["Postgres"]["Schema"];
    return schema.empty() ? postgres_dao_modules::DefaultSchema() : schema;
}

int PostgresPoolSize()
{
    auto& cfg = ConfigMgr::Inst();
    const auto configured = cfg["Postgres"]["PoolSize"];
    int parsed = 0;
    const auto [ptr, ec] = std::from_chars(configured.data(), configured.data() + configured.size(), parsed);
    if (ec != std::errc{} || ptr != configured.data() + configured.size())
    {
        parsed = 0;
    }
    return postgres_dao_modules::NormalizePoolSize(configured.empty(), parsed);
}
} // namespace

PostgresDao::PostgresDao()
{
    pool_.reset(new PostgresPool(BuildConnectionString(), PostgresSchemaName(), PostgresPoolSize()));
    if (!pool_ || !pool_->Ready())
    {
        startup_error_ = pool_ ? pool_->StartupError() : "postgres pool allocation failed";
        return;
    }
    account_connection_string_ = BuildAccountConnectionString();
    WarmupAuthQueries();
    ready_ = true;
}

PostgresDao::~PostgresDao()
{
    if (pool_)
    {
        pool_->Close();
    }
}

bool PostgresDao::Ready() const noexcept
{
    return ready_;
}

const std::string& PostgresDao::StartupError() const noexcept
{
    return startup_error_;
}

bool PostgresDao::UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const auto result = txn.exec_params("UPDATE \"user\" SET nick = $1, \"desc\" = $2, icon = $3 WHERE uid = $4",
                                        nick,
                                        desc,
                                        icon,
                                        uid);
    if (!TransactionOk("UpdateUserProfile", txn))
    {
        return false;
    }
    if (result.affected_rows() > 0)
    {
        return txn.commit();
    }

    const auto rows = txn.exec_params("SELECT 1 FROM \"user\" WHERE uid = $1 LIMIT 1", uid);
    const bool exists = TransactionOk("UpdateUserProfile", txn) && !rows.empty();
    return txn.commit() && exists;
}

bool PostgresDao::GetCallUserProfile(int uid, CallUserProfile& profile)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows =
        txn.exec_params("SELECT uid, user_id, name, nick, icon FROM \"user\" WHERE uid = $1 LIMIT 1", uid);
    if (!TransactionOk("GetCallUserProfile", txn) || rows.empty())
    {
        return false;
    }

    const auto& row = rows[0];
    profile.uid = row["uid"].as<int>();
    profile.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
    profile.name = row["name"].is_null() ? "" : row["name"].c_str();
    profile.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
    profile.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
    return TransactionOk("GetCallUserProfile", txn);
}

bool PostgresDao::IsFriend(int uid, int peer_uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows =
        txn.exec_params("SELECT 1 FROM friend WHERE self_id = $1 AND friend_id = $2 LIMIT 1", uid, peer_uid);
    return TransactionOk("IsFriend", txn) && !rows.empty();
}

bool PostgresDao::UpsertCallSession(const CallSessionInfo& session)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const auto result = txn.exec_params(
        "INSERT INTO chat_call_session("
        "call_id, room_name, call_type, caller_uid, callee_uid, state, "
        "started_at_ms, accepted_at_ms, ended_at_ms, expires_at_ms, duration_sec, reason, trace_id, updated_at_ms"
        ") VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14) "
        "ON CONFLICT (call_id) DO UPDATE SET "
        "room_name = EXCLUDED.room_name, call_type = EXCLUDED.call_type, caller_uid = EXCLUDED.caller_uid, "
        "callee_uid = EXCLUDED.callee_uid, state = EXCLUDED.state, started_at_ms = EXCLUDED.started_at_ms, "
        "accepted_at_ms = EXCLUDED.accepted_at_ms, ended_at_ms = EXCLUDED.ended_at_ms, "
        "expires_at_ms = EXCLUDED.expires_at_ms, duration_sec = EXCLUDED.duration_sec, "
        "reason = EXCLUDED.reason, trace_id = EXCLUDED.trace_id, updated_at_ms = EXCLUDED.updated_at_ms",
        session.call_id,
        session.room_name,
        session.call_type,
        session.caller_uid,
        session.callee_uid,
        session.state,
        session.started_at_ms,
        session.accepted_at_ms,
        session.ended_at_ms,
        session.expires_at_ms,
        session.duration_sec,
        session.reason,
        session.trace_id,
        session.updated_at_ms);
    const bool changed = TransactionOk("UpsertCallSession", txn) && result.affected_rows() > 0;
    return txn.commit() && changed;
}

bool PostgresDao::GetCallSession(const std::string& call_id, CallSessionInfo& session)
{
    if (call_id.empty())
    {
        return false;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params(
        "SELECT call_id, room_name, call_type, caller_uid, callee_uid, state, "
        "started_at_ms, accepted_at_ms, ended_at_ms, expires_at_ms, duration_sec, reason, trace_id, updated_at_ms "
        "FROM chat_call_session WHERE call_id = $1 LIMIT 1",
        call_id);
    if (!TransactionOk("GetCallSession", txn) || rows.empty())
    {
        return false;
    }

    const auto& row = rows[0];
    session.call_id = row["call_id"].is_null() ? "" : row["call_id"].c_str();
    session.room_name = row["room_name"].is_null() ? "" : row["room_name"].c_str();
    session.call_type = row["call_type"].is_null() ? "" : row["call_type"].c_str();
    session.caller_uid = row["caller_uid"].is_null() ? 0 : row["caller_uid"].as<int>();
    session.callee_uid = row["callee_uid"].is_null() ? 0 : row["callee_uid"].as<int>();
    session.state = row["state"].is_null() ? "" : row["state"].c_str();
    session.started_at_ms = row["started_at_ms"].is_null() ? 0 : row["started_at_ms"].as<int64_t>();
    session.accepted_at_ms = row["accepted_at_ms"].is_null() ? 0 : row["accepted_at_ms"].as<int64_t>();
    session.ended_at_ms = row["ended_at_ms"].is_null() ? 0 : row["ended_at_ms"].as<int64_t>();
    session.expires_at_ms = row["expires_at_ms"].is_null() ? 0 : row["expires_at_ms"].as<int64_t>();
    session.duration_sec = row["duration_sec"].is_null() ? 0 : row["duration_sec"].as<int>();
    session.reason = row["reason"].is_null() ? "" : row["reason"].c_str();
    session.trace_id = row["trace_id"].is_null() ? "" : row["trace_id"].c_str();
    session.updated_at_ms = row["updated_at_ms"].is_null() ? 0 : row["updated_at_ms"].as<int64_t>();
    return TransactionOk("GetCallSession", txn);
}

bool PostgresDao::InsertMediaAsset(const MediaAssetInfo& asset)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const auto result = txn.exec_params("INSERT INTO chat_media_asset("
                                        "media_key, owner_uid, media_type, origin_file_name, mime, size_bytes, "
                                        "storage_provider, storage_path, created_at_ms, deleted_at_ms, status"
                                        ") VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11)",
                                        asset.media_key,
                                        asset.owner_uid,
                                        asset.media_type,
                                        asset.origin_file_name,
                                        asset.mime,
                                        asset.size_bytes,
                                        asset.storage_provider,
                                        asset.storage_path,
                                        asset.created_at_ms,
                                        asset.deleted_at_ms,
                                        asset.status);
    const bool inserted = TransactionOk("InsertMediaAsset", txn) && result.affected_rows() > 0;
    return txn.commit() && inserted;
}

bool PostgresDao::GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset)
{
    if (media_key.empty())
    {
        return false;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows =
        txn.exec_params("SELECT media_id, media_key, owner_uid, media_type, origin_file_name, mime, size_bytes, "
                        "storage_provider, storage_path, created_at_ms, deleted_at_ms, status "
                        "FROM chat_media_asset WHERE media_key = $1 LIMIT 1",
                        media_key);
    if (!TransactionOk("GetMediaAssetByKey", txn) || rows.empty())
    {
        return false;
    }

    const auto& row = rows[0];
    asset.media_id = row["media_id"].as<int64_t>();
    asset.media_key = row["media_key"].is_null() ? "" : row["media_key"].c_str();
    asset.owner_uid = row["owner_uid"].is_null() ? 0 : row["owner_uid"].as<int>();
    asset.media_type = row["media_type"].is_null() ? "" : row["media_type"].c_str();
    asset.origin_file_name = row["origin_file_name"].is_null() ? "" : row["origin_file_name"].c_str();
    asset.mime = row["mime"].is_null() ? "" : row["mime"].c_str();
    asset.size_bytes = row["size_bytes"].is_null() ? 0 : row["size_bytes"].as<int64_t>();
    asset.storage_provider = row["storage_provider"].is_null() ? "" : row["storage_provider"].c_str();
    asset.storage_path = row["storage_path"].is_null() ? "" : row["storage_path"].c_str();
    asset.created_at_ms = row["created_at_ms"].is_null() ? 0 : row["created_at_ms"].as<int64_t>();
    asset.deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
    asset.status = row["status"].is_null() ? 0 : row["status"].as<int>();
    return TransactionOk("GetMediaAssetByKey", txn);
}

bool PostgresDao::GrantMediaAccess(int64_t media_id, int grantee_uid, const std::string& scope, int64_t created_at_ms)
{
    if (media_id <= 0 || grantee_uid < 0 || scope.empty())
    {
        return false;
    }
    if (grantee_uid == 0 && scope != "public" && scope != "friends")
    {
        return false;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const auto rows =
        txn.exec_params("INSERT INTO chat_media_access_grant(media_id, grantee_uid, grant_scope, created_at_ms) "
                        "SELECT $1, $2, $3, $4 WHERE EXISTS ("
                        "    SELECT 1 FROM chat_media_asset WHERE media_id = $1 AND status = 1 AND deleted_at_ms = 0"
                        ") "
                        "ON CONFLICT (media_id, grantee_uid, grant_scope) DO UPDATE "
                        "SET revoked_at_ms = 0, created_at_ms = EXCLUDED.created_at_ms",
                        media_id,
                        grantee_uid,
                        scope,
                        created_at_ms);
    const bool granted = TransactionOk("GrantMediaAccess", txn) && rows.affected_rows() > 0;
    return txn.commit() && granted;
}

bool PostgresDao::GrantMediaGroupAccess(int64_t media_id,
                                        int64_t group_id,
                                        int owner_uid,
                                        const std::string& scope,
                                        int64_t created_at_ms)
{
    if (media_id <= 0 || group_id <= 0 || owner_uid <= 0 || scope.empty())
    {
        return false;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const std::string group_scope = scope + ":" + std::to_string(group_id);
    const auto rows =
        txn.exec_params("INSERT INTO chat_media_access_grant(media_id, grantee_uid, grant_scope, created_at_ms) "
                        "SELECT $1, 0, $3, $4 WHERE EXISTS ("
                        "    SELECT 1 FROM chat_media_asset "
                        "    WHERE media_id = $1 AND owner_uid = $2 AND status = 1 AND deleted_at_ms = 0"
                        ") "
                        "ON CONFLICT (media_id, grantee_uid, grant_scope) DO UPDATE "
                        "SET revoked_at_ms = 0, created_at_ms = EXCLUDED.created_at_ms",
                        media_id,
                        owner_uid,
                        group_scope,
                        created_at_ms);
    const bool granted = TransactionOk("GrantMediaGroupAccess", txn) && rows.affected_rows() > 0;
    return txn.commit() && granted;
}

bool PostgresDao::HasMediaAccess(int64_t media_id, int uid)
{
    if (media_id <= 0 || uid <= 0)
    {
        return false;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto avatar_rows = txn.exec_params("SELECT 1 FROM chat_media_asset "
                                             "WHERE media_id = $1 AND media_type = 'avatar' "
                                             "  AND status = 1 AND deleted_at_ms = 0 "
                                             "LIMIT 1",
                                             media_id);
    if (!TransactionOk("HasMediaAccess", txn))
    {
        return false;
    }
    if (!avatar_rows.empty())
    {
        return true;
    }

    const auto direct_rows = txn.exec_params("SELECT 1 FROM chat_media_access_grant "
                                             "WHERE media_id = $1 AND revoked_at_ms = 0 "
                                             "  AND (grantee_uid = $2 OR (grantee_uid = 0 AND grant_scope = 'public')) "
                                             "LIMIT 1",
                                             media_id,
                                             uid);
    if (!TransactionOk("HasMediaAccess", txn))
    {
        return false;
    }
    if (!direct_rows.empty())
    {
        return true;
    }

    const auto group_table_rows = txn.exec("SELECT to_regclass('chat_group_member') IS NOT NULL");
    if (!TransactionOk("HasMediaAccess", txn))
    {
        return false;
    }
    if (!group_table_rows.empty() && !group_table_rows[0][0].is_null() && group_table_rows[0][0].as<bool>())
    {
        const auto group_rows = txn.exec_params("SELECT 1 "
                                                "FROM chat_media_access_grant grant_row "
                                                "JOIN chat_group_member member "
                                                "  ON grant_row.grant_scope = ('group:' || member.group_id::text) "
                                                "WHERE grant_row.media_id = $1 AND grant_row.grantee_uid = 0 "
                                                "  AND grant_row.revoked_at_ms = 0 "
                                                "  AND member.uid = $2 AND member.status = 1 "
                                                "LIMIT 1",
                                                media_id,
                                                uid);
        if (!TransactionOk("HasMediaAccess", txn))
        {
            return false;
        }
        if (!group_rows.empty())
        {
            return true;
        }
    }

    const auto friends_grant_rows =
        txn.exec_params("SELECT 1 FROM chat_media_access_grant "
                        "WHERE media_id = $1 AND grantee_uid = 0 AND grant_scope = 'friends' "
                        "  AND revoked_at_ms = 0 LIMIT 1",
                        media_id);
    if (!TransactionOk("HasMediaAccess", txn) || friends_grant_rows.empty())
    {
        return false;
    }

    const auto friend_table_rows = txn.exec("SELECT to_regclass('friend') IS NOT NULL");
    if (!TransactionOk("HasMediaAccess", txn) || friend_table_rows.empty() || friend_table_rows[0][0].is_null() ||
        !friend_table_rows[0][0].as<bool>())
    {
        return false;
    }

    const auto friends_rows =
        txn.exec_params("SELECT 1 "
                        "FROM chat_media_asset asset "
                        "WHERE asset.media_id = $1 AND asset.status = 1 AND asset.deleted_at_ms = 0 "
                        "  AND EXISTS ("
                        "      SELECT 1 FROM friend f "
                        "      WHERE ((f.self_id = asset.owner_uid AND f.friend_id = $2) "
                        "          OR (f.self_id = $2 AND f.friend_id = asset.owner_uid))"
                        "  ) "
                        "LIMIT 1",
                        media_id,
                        uid);
    return TransactionOk("HasMediaAccess", txn) && !friends_rows.empty();
}

bool PostgresDao::AddMoment(const MomentInfo& moment, int64_t* moment_id)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    pqxx::result rows =
        txn.exec_params("INSERT INTO moments(uid, visibility, location, created_at, like_count, comment_count) "
                        "VALUES ($1, $2, $3, $4, $5, $6) RETURNING moment_id",
                        moment.uid,
                        moment.visibility,
                        moment.location,
                        moment.created_at,
                        moment.like_count,
                        moment.comment_count);
    if (!TransactionOk("AddMoment", txn) || rows.empty())
    {
        return false;
    }
    const int64_t inserted_id = rows[0]["moment_id"].as<int64_t>();
    if (!TransactionOk("AddMoment", txn) || !txn.commit())
    {
        return false;
    }
    if (moment_id)
    {
        *moment_id = inserted_id;
    }
    return true;
}

bool PostgresDao::GetMomentsFeed(int viewer_uid,
                                 int64_t last_moment_id,
                                 int limit,
                                 int author_uid,
                                 std::vector<MomentInfo>& moments,
                                 bool& has_more)
{
    moments.clear();
    has_more = false;
    if (limit <= 0)
    {
        limit = 20;
    }
    limit = std::min(limit, 50);

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);

    pqxx::result rows;
    if (last_moment_id <= 0)
    {
        rows = txn.exec_params(R"(
				SELECT m.moment_id, m.uid, m.visibility, m.location, m.created_at,
				       m.like_count, m.comment_count
				FROM moments m
				WHERE m.deleted_at = 0
				  AND ($3 <= 0 OR m.uid = $3)
				  AND (
					m.visibility = 0
					OR m.uid = $1
					OR (
						m.visibility = 1
						AND (
							EXISTS (
								SELECT 1
								FROM friend f
								WHERE ((f.self_id = m.uid AND f.friend_id = $1)
									OR (f.self_id = $1 AND f.friend_id = m.uid))
							)
							OR EXISTS (
								SELECT 1
								FROM friend_apply fa
								WHERE fa.status = 1
								  AND ((fa.from_uid = m.uid AND fa.to_uid = $1)
									OR (fa.from_uid = $1 AND fa.to_uid = m.uid))
							)
						)
					)
				  )
				ORDER BY m.created_at DESC, m.moment_id DESC
				LIMIT $2
			)",
                               viewer_uid,
                               limit + 1,
                               author_uid);
    }
    else
    {
        rows = txn.exec_params(R"(
				SELECT m.moment_id, m.uid, m.visibility, m.location, m.created_at,
				       m.like_count, m.comment_count
				FROM moments m
				WHERE m.deleted_at = 0
				  AND m.moment_id < $2
				  AND ($4 <= 0 OR m.uid = $4)
				  AND (
					m.visibility = 0
					OR m.uid = $1
					OR (
						m.visibility = 1
						AND (
							EXISTS (
								SELECT 1
								FROM friend f
								WHERE ((f.self_id = m.uid AND f.friend_id = $1)
									OR (f.self_id = $1 AND f.friend_id = m.uid))
							)
							OR EXISTS (
								SELECT 1
								FROM friend_apply fa
								WHERE fa.status = 1
								  AND ((fa.from_uid = m.uid AND fa.to_uid = $1)
									OR (fa.from_uid = $1 AND fa.to_uid = m.uid))
							)
						)
					)
				  )
				ORDER BY m.created_at DESC, m.moment_id DESC
				LIMIT $3
			)",
                               viewer_uid,
                               last_moment_id,
                               limit + 1,
                               author_uid);
    }

    if (!TransactionOk("GetMomentsFeed", txn))
    {
        return false;
    }

    if (static_cast<int>(rows.size()) > limit)
    {
        has_more = true;
    }

    for (size_t i = 0; i < rows.size() && static_cast<int>(moments.size()) < limit; ++i)
    {
        const auto& row = rows[i];
        MomentInfo info;
        info.moment_id = row["moment_id"].as<int64_t>();
        info.uid = row["uid"].as<int>();
        info.visibility = row["visibility"].as<int>();
        info.location = row["location"].is_null() ? "" : row["location"].c_str();
        info.created_at = row["created_at"].as<int64_t>();
        info.like_count = row["like_count"].as<int>();
        info.comment_count = row["comment_count"].as<int>();
        moments.push_back(info);
    }

    if (!TransactionOk("GetMomentsFeed", txn))
    {
        moments.clear();
        return false;
    }
    return true;
}

bool PostgresDao::GetMomentsFeedCandidates(int viewer_uid,
                                           int64_t last_moment_id,
                                           int limit,
                                           int author_uid,
                                           std::vector<MomentInfo>& moments,
                                           bool& has_more)
{
    // Like GetMomentsFeed but WITHOUT the friend/friend_apply EXISTS subquery,
    // so it runs in a database that doesn't own the friend tables (memo_moments
    // after the Phase 2 split). It returns every friends-only (visibility=1)
    // moment as a candidate; the caller must drop authors that aren't friends
    // of viewer_uid using the relation service (FilterFriendUids). Public
    // (visibility=0) and own moments are already fully resolved here.
    moments.clear();
    has_more = false;
    if (limit <= 0)
    {
        limit = 20;
    }
    limit = std::min(limit, 50);

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);

    pqxx::result rows;
    if (last_moment_id <= 0)
    {
        rows = txn.exec_params(R"(
                SELECT m.moment_id, m.uid, m.visibility, m.location, m.created_at,
                       m.like_count, m.comment_count
                FROM moments m
                WHERE m.deleted_at = 0
                  AND ($3 <= 0 OR m.uid = $3)
                  AND (m.visibility = 0 OR m.uid = $1 OR m.visibility = 1)
                ORDER BY m.created_at DESC, m.moment_id DESC
                LIMIT $2
            )",
                               viewer_uid,
                               limit + 1,
                               author_uid);
    }
    else
    {
        rows = txn.exec_params(R"(
                SELECT m.moment_id, m.uid, m.visibility, m.location, m.created_at,
                       m.like_count, m.comment_count
                FROM moments m
                WHERE m.deleted_at = 0
                  AND m.moment_id < $2
                  AND ($4 <= 0 OR m.uid = $4)
                  AND (m.visibility = 0 OR m.uid = $1 OR m.visibility = 1)
                ORDER BY m.created_at DESC, m.moment_id DESC
                LIMIT $3
            )",
                               viewer_uid,
                               last_moment_id,
                               limit + 1,
                               author_uid);
    }

    if (!TransactionOk("GetMomentsFeedCandidates", txn))
    {
        return false;
    }

    if (static_cast<int>(rows.size()) > limit)
    {
        has_more = true;
    }

    for (size_t i = 0; i < rows.size() && static_cast<int>(moments.size()) < limit; ++i)
    {
        const auto& row = rows[i];
        MomentInfo info;
        info.moment_id = row["moment_id"].as<int64_t>();
        info.uid = row["uid"].as<int>();
        info.visibility = row["visibility"].as<int>();
        info.location = row["location"].is_null() ? "" : row["location"].c_str();
        info.created_at = row["created_at"].as<int64_t>();
        info.like_count = row["like_count"].as<int>();
        info.comment_count = row["comment_count"].as<int>();
        moments.push_back(info);
    }

    if (!TransactionOk("GetMomentsFeedCandidates", txn))
    {
        moments.clear();
        return false;
    }
    return true;
}

bool PostgresDao::CanViewMoment(int viewer_uid, const MomentInfo& moment)
{
    if (viewer_uid <= 0 || moment.uid <= 0)
    {
        return false;
    }
    if (viewer_uid == moment.uid)
    {
        return true;
    }
    if (moment.visibility == 0)
    {
        return true;
    }
    if (moment.visibility != 1)
    {
        return false;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows =
        txn.exec_params("SELECT 1 "
                        "WHERE EXISTS ("
                        "    SELECT 1 FROM friend f "
                        "    WHERE ((f.self_id = $1 AND f.friend_id = $2) OR (f.self_id = $2 AND f.friend_id = $1))"
                        ") "
                        "OR EXISTS ("
                        "    SELECT 1 FROM friend_apply fa "
                        "    WHERE fa.status = 1 "
                        "      AND ((fa.from_uid = $1 AND fa.to_uid = $2) OR (fa.from_uid = $2 AND fa.to_uid = $1))"
                        ")",
                        viewer_uid,
                        moment.uid);
    return TransactionOk("CanViewMoment", txn) && !rows.empty();
}

bool PostgresDao::GetMomentById(int64_t moment_id, MomentInfo& moment)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params(
        "SELECT moment_id, uid, visibility, location, created_at, deleted_at, like_count, comment_count "
        "FROM moments WHERE moment_id = $1 AND deleted_at = 0 LIMIT 1",
        moment_id);
    if (!TransactionOk("GetMomentById", txn) || rows.empty())
    {
        return false;
    }

    const auto& row = rows[0];
    moment.moment_id = row["moment_id"].as<int64_t>();
    moment.uid = row["uid"].as<int>();
    moment.visibility = row["visibility"].as<int>();
    moment.location = row["location"].is_null() ? "" : row["location"].c_str();
    moment.created_at = row["created_at"].as<int64_t>();
    moment.deleted_at = row["deleted_at"].as<int64_t>();
    moment.like_count = row["like_count"].as<int>();
    moment.comment_count = row["comment_count"].as<int>();
    return TransactionOk("GetMomentById", txn);
}

bool PostgresDao::DeleteMoment(int64_t moment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    auto now =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto rows =
        txn.exec_params("UPDATE moments SET deleted_at = $1 WHERE moment_id = $2 AND uid = $3", now, moment_id, uid);
    const bool deleted = TransactionOk("DeleteMoment", txn) && rows.affected_rows() > 0;
    return txn.commit() && deleted;
}

bool PostgresDao::AddMomentLike(int64_t moment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const auto inserted = txn.exec_params("INSERT INTO moments_like(moment_id, uid, created_at) "
                                          "SELECT $1, $2, $3 WHERE EXISTS ("
                                          "    SELECT 1 FROM moments WHERE moment_id = $1 AND deleted_at = 0"
                                          ") "
                                          "ON CONFLICT (moment_id, uid) DO NOTHING",
                                          moment_id,
                                          uid,
                                          static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                                                   std::chrono::system_clock::now().time_since_epoch())
                                                                   .count()));
    if (TransactionOk("AddMomentLike", txn) && inserted.affected_rows() > 0)
    {
        txn.exec_params("UPDATE moments SET like_count = like_count + 1 WHERE moment_id = $1 AND deleted_at = 0",
                        moment_id);
    }
    return TransactionOk("AddMomentLike", txn) && txn.commit();
}

bool PostgresDao::RemoveMomentLike(int64_t moment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const auto deleted = txn.exec_params("DELETE FROM moments_like ml USING moments m "
                                         "WHERE ml.moment_id = $1 AND ml.uid = $2 "
                                         "AND m.moment_id = ml.moment_id AND m.deleted_at = 0",
                                         moment_id,
                                         uid);
    if (TransactionOk("RemoveMomentLike", txn) && deleted.affected_rows() > 0)
    {
        txn.exec_params("UPDATE moments SET like_count = GREATEST(like_count - 1, 0) "
                        "WHERE moment_id = $1 AND deleted_at = 0",
                        moment_id);
    }
    return TransactionOk("RemoveMomentLike", txn) && txn.commit();
}

bool PostgresDao::HasLikedMoment(int64_t moment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT 1 FROM moments_like ml "
                                      "JOIN moments m ON m.moment_id = ml.moment_id "
                                      "WHERE ml.moment_id = $1 AND ml.uid = $2 AND m.deleted_at = 0 LIMIT 1",
                                      moment_id,
                                      uid);
    return TransactionOk("HasLikedMoment", txn) && !rows.empty();
}

bool PostgresDao::GetMomentLikes(int64_t moment_id, int limit, std::vector<MomentLikeInfo>& likes, bool& has_more)
{
    likes.clear();
    has_more = false;
    if (limit <= 0)
    {
        limit = 20;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT ml.id, ml.moment_id, ml.uid, ml.created_at "
                                      "FROM moments_like ml "
                                      "JOIN moments m ON m.moment_id = ml.moment_id "
                                      "WHERE ml.moment_id = $1 AND m.deleted_at = 0 "
                                      "ORDER BY ml.created_at DESC LIMIT $2",
                                      moment_id,
                                      limit + 1);

    if (!TransactionOk("GetMomentLikes", txn))
    {
        return false;
    }
    if (static_cast<int>(rows.size()) > limit)
    {
        has_more = true;
    }

    for (size_t i = 0; i < rows.size() && static_cast<int>(likes.size()) < limit; ++i)
    {
        const auto& row = rows[i];
        MomentLikeInfo info;
        info.id = row["id"].as<int64_t>();
        info.moment_id = row["moment_id"].as<int64_t>();
        info.uid = row["uid"].as<int>();
        info.created_at = row["created_at"].as<int64_t>();
        likes.push_back(info);
    }
    if (!TransactionOk("GetMomentLikes", txn))
    {
        likes.clear();
        return false;
    }

    std::vector<int> uids;
    for (const auto& lk : likes)
        uids.push_back(lk.uid);
    const auto profiles = FetchUserProfiles(account_connection_string_, uids);
    for (auto& lk : likes)
    {
        auto it = profiles.find(lk.uid);
        if (it != profiles.end())
        {
            lk.user_nick = it->second.first;
            lk.user_icon = it->second.second;
        }
    }
    return true;
}

bool PostgresDao::AddMomentComment(const MomentCommentInfo& comment)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    const auto inserted = txn.exec_params("INSERT INTO moments_comment(moment_id, uid, content, reply_uid, created_at) "
                                          "SELECT $1, $2, $3, $4, $5 WHERE EXISTS ("
                                          "    SELECT 1 FROM moments WHERE moment_id = $1 AND deleted_at = 0"
                                          ")",
                                          comment.moment_id,
                                          comment.uid,
                                          comment.content,
                                          comment.reply_uid,
                                          comment.created_at);
    if (!TransactionOk("AddMomentComment", txn) || inserted.affected_rows() <= 0)
    {
        txn.abort();
        return false;
    }
    txn.exec_params("UPDATE moments SET comment_count = comment_count + 1 WHERE moment_id = $1 AND deleted_at = 0",
                    comment.moment_id);
    return TransactionOk("AddMomentComment", txn) && txn.commit();
}

bool PostgresDao::DeleteMomentComment(int64_t comment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    auto now =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto rows = txn.exec_params(
        "UPDATE moments_comment mc SET deleted_at = $1 "
        "WHERE mc.id = $2 AND mc.deleted_at = 0 "
        "AND (mc.uid = $3 OR EXISTS ("
        "    SELECT 1 FROM moments m WHERE m.moment_id = mc.moment_id AND m.uid = $3 AND m.deleted_at = 0"
        "))",
        now,
        comment_id,
        uid);
    if (TransactionOk("DeleteMomentComment", txn) && rows.affected_rows() > 0)
    {
        txn.exec_params("DELETE FROM moments_comment_like WHERE comment_id = $1", comment_id);
        txn.exec_params("UPDATE moments SET comment_count = GREATEST(comment_count - 1, 0) "
                        "WHERE moment_id = (SELECT moment_id FROM moments_comment WHERE id = $1)",
                        comment_id);
    }
    const bool deleted = TransactionOk("DeleteMomentComment", txn) && rows.affected_rows() > 0;
    return txn.commit() && deleted;
}

bool PostgresDao::GetMomentCommentById(int64_t comment_id, MomentCommentInfo& comment)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows =
        txn.exec_params("SELECT mc.id, mc.moment_id, mc.uid, mc.content, mc.reply_uid, mc.created_at, mc.deleted_at "
                        "FROM moments_comment mc "
                        "JOIN moments m ON m.moment_id = mc.moment_id "
                        "WHERE mc.id = $1 AND (mc.deleted_at = 0 OR mc.deleted_at IS NULL) "
                        "AND m.deleted_at = 0 LIMIT 1",
                        comment_id);
    if (!TransactionOk("GetMomentCommentById", txn) || rows.empty())
    {
        return false;
    }

    const auto& row = rows[0];
    comment = MomentCommentInfo{};
    comment.id = row["id"].as<int64_t>();
    comment.moment_id = row["moment_id"].as<int64_t>();
    comment.uid = row["uid"].as<int>();
    comment.content = row["content"].is_null() ? "" : row["content"].c_str();
    comment.reply_uid = row["reply_uid"].as<int>();
    comment.created_at = row["created_at"].as<int64_t>();
    comment.deleted_at = row["deleted_at"].is_null() ? 0 : row["deleted_at"].as<int64_t>();
    return TransactionOk("GetMomentCommentById", txn);
}

bool PostgresDao::GetMomentComments(int64_t moment_id,
                                    int64_t last_comment_id,
                                    int limit,
                                    std::vector<MomentCommentInfo>& comments,
                                    bool& has_more)
{
    comments.clear();
    has_more = false;
    if (limit <= 0)
    {
        limit = 20;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    pqxx::result rows;
    if (last_comment_id <= 0)
    {
        rows = txn.exec_params(
            "SELECT mc.id, mc.moment_id, mc.uid, mc.content, mc.reply_uid, mc.created_at, mc.deleted_at "
            "FROM moments_comment mc "
            "JOIN moments m ON m.moment_id = mc.moment_id "
            "WHERE mc.moment_id = $1 AND (mc.deleted_at = 0 OR mc.deleted_at IS NULL) "
            "AND m.deleted_at = 0 "
            "ORDER BY mc.created_at ASC LIMIT $2",
            moment_id,
            limit + 1);
    }
    else
    {
        rows = txn.exec_params(
            "SELECT mc.id, mc.moment_id, mc.uid, mc.content, mc.reply_uid, mc.created_at, mc.deleted_at "
            "FROM moments_comment mc "
            "JOIN moments m ON m.moment_id = mc.moment_id "
            "WHERE mc.moment_id = $1 AND (mc.deleted_at = 0 OR mc.deleted_at IS NULL) "
            "AND m.deleted_at = 0 AND mc.id > $2 "
            "ORDER BY mc.created_at ASC LIMIT $3",
            moment_id,
            last_comment_id,
            limit + 1);
    }

    if (!TransactionOk("GetMomentComments", txn))
    {
        return false;
    }
    if (static_cast<int>(rows.size()) > limit)
    {
        has_more = true;
    }

    for (size_t i = 0; i < rows.size() && static_cast<int>(comments.size()) < limit; ++i)
    {
        const auto& row = rows[i];
        MomentCommentInfo info;
        info.id = row["id"].as<int64_t>();
        info.moment_id = row["moment_id"].as<int64_t>();
        info.uid = row["uid"].as<int>();
        info.content = row["content"].is_null() ? "" : row["content"].c_str();
        info.reply_uid = row["reply_uid"].as<int>();
        info.created_at = row["created_at"].as<int64_t>();
        info.deleted_at = row["deleted_at"].as<int64_t>();
        comments.push_back(info);
    }
    if (!TransactionOk("GetMomentComments", txn))
    {
        comments.clear();
        return false;
    }

    std::vector<int> uids;
    for (const auto& cm : comments)
    {
        uids.push_back(cm.uid);
        if (cm.reply_uid > 0)
            uids.push_back(cm.reply_uid);
    }
    const auto profiles = FetchUserProfiles(account_connection_string_, uids);
    for (auto& cm : comments)
    {
        auto it = profiles.find(cm.uid);
        if (it != profiles.end())
        {
            cm.user_nick = it->second.first;
            cm.user_icon = it->second.second;
        }
        if (cm.reply_uid > 0)
        {
            auto rit = profiles.find(cm.reply_uid);
            if (rit != profiles.end())
                cm.reply_nick = rit->second.first;
        }
    }
    return true;
}

bool PostgresDao::AddMomentCommentLike(int64_t comment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    txn.exec_params("INSERT INTO moments_comment_like(comment_id, uid, created_at) "
                    "SELECT $1, $2, $3 "
                    "WHERE EXISTS ("
                    "    SELECT 1 FROM moments_comment mc "
                    "    JOIN moments m ON m.moment_id = mc.moment_id "
                    "    WHERE mc.id = $1 AND mc.deleted_at = 0 AND m.deleted_at = 0"
                    ") "
                    "ON CONFLICT (comment_id, uid) DO NOTHING",
                    comment_id,
                    uid,
                    static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                             std::chrono::system_clock::now().time_since_epoch())
                                             .count()));
    return TransactionOk("AddMomentCommentLike", txn) && txn.commit();
}

bool PostgresDao::RemoveMomentCommentLike(int64_t comment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::work txn(*con->_con);
    txn.exec_params("DELETE FROM moments_comment_like mcl USING moments_comment mc, moments m "
                    "WHERE mcl.comment_id = $1 AND mcl.uid = $2 "
                    "AND mc.id = mcl.comment_id AND mc.deleted_at = 0 "
                    "AND m.moment_id = mc.moment_id AND m.deleted_at = 0",
                    comment_id,
                    uid);
    return TransactionOk("RemoveMomentCommentLike", txn) && txn.commit();
}

bool PostgresDao::HasLikedMomentComment(int64_t comment_id, int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT 1 FROM moments_comment_like mcl "
                                      "JOIN moments_comment mc ON mc.id = mcl.comment_id "
                                      "JOIN moments m ON m.moment_id = mc.moment_id "
                                      "WHERE mcl.comment_id = $1 AND mcl.uid = $2 "
                                      "AND mc.deleted_at = 0 AND m.deleted_at = 0 LIMIT 1",
                                      comment_id,
                                      uid);
    return TransactionOk("HasLikedMomentComment", txn) && !rows.empty();
}

bool PostgresDao::GetMomentCommentLikes(int64_t comment_id,
                                        int limit,
                                        std::vector<MomentLikeInfo>& likes,
                                        bool& has_more)
{
    likes.clear();
    has_more = false;
    if (limit <= 0)
    {
        limit = 20;
    }

    auto con = pool_->getConnection();
    if (con == nullptr)
    {
        return false;
    }

    Defer defer(
        [this, &con]()
        {
            pool_->returnConnection(std::move(con));
        });

    pqxx::read_transaction txn(*con->_con);
    const auto rows = txn.exec_params("SELECT mcl.id, mcl.comment_id, mcl.uid, mcl.created_at "
                                      "FROM moments_comment_like mcl "
                                      "JOIN moments_comment mc ON mc.id = mcl.comment_id "
                                      "JOIN moments m ON m.moment_id = mc.moment_id "
                                      "WHERE mcl.comment_id = $1 "
                                      "AND mc.deleted_at = 0 AND m.deleted_at = 0 "
                                      "ORDER BY mcl.created_at DESC LIMIT $2",
                                      comment_id,
                                      limit + 1);

    if (!TransactionOk("GetMomentCommentLikes", txn))
    {
        return false;
    }
    if (static_cast<int>(rows.size()) > limit)
    {
        has_more = true;
    }

    for (size_t i = 0; i < rows.size() && static_cast<int>(likes.size()) < limit; ++i)
    {
        const auto& row = rows[i];
        MomentLikeInfo info;
        info.id = row["id"].as<int64_t>();
        info.moment_id = row["comment_id"].as<int64_t>();
        info.uid = row["uid"].as<int>();
        info.created_at = row["created_at"].as<int64_t>();
        likes.push_back(info);
    }
    if (!TransactionOk("GetMomentCommentLikes", txn))
    {
        likes.clear();
        return false;
    }

    std::vector<int> uids;
    for (const auto& lk : likes)
        uids.push_back(lk.uid);
    const auto profiles = FetchUserProfiles(account_connection_string_, uids);
    for (auto& lk : likes)
    {
        auto it = profiles.find(lk.uid);
        if (it != profiles.end())
        {
            lk.user_nick = it->second.first;
            lk.user_icon = it->second.second;
        }
    }
    return true;
}
