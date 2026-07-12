#include "db/AISmartLogRepo.hpp"
#include "ConfigMgr.hpp"
#include "db/PqxxCompat.hpp"
#include "logging/Logger.hpp"
#include <chrono>
#include <sstream>

import memochat.ai.repository_algorithms;

namespace
{
namespace repository_modules = memochat::ai::repository::modules;

std::unique_ptr<pqxx::connection> GetPgConn(std::string& error)
{
    auto& cfg = ConfigMgr::Inst();
    const auto schema = repository_modules::ShouldUseDefaultSchema(cfg["Postgres"]["Schema"].empty())
                            ? repository_modules::DefaultPostgresSchema()
                            : cfg["Postgres"]["Schema"];
    std::ostringstream conn_str;
    conn_str << "host=" << cfg["Postgres"]["Host"] << " port=" << cfg["Postgres"]["Port"]
             << " user=" << cfg["Postgres"]["User"] << " password=" << cfg["Postgres"]["Passwd"]
             << " dbname=" << cfg["Postgres"]["Database"];
    auto conn = std::make_unique<pqxx::connection>(conn_str.str());
    if (!conn->is_open())
    {
        error = conn->error_message();
        return nullptr;
    }
    pqxx::work tx(*conn);
    const auto configured = tx.exec0("SET search_path TO " + tx.quote_name(schema) + ",public");
    if (!configured.ok() || !tx.commit())
    {
        error = configured.ok() ? tx.error_message() : configured.error_message();
        return nullptr;
    }
    return conn;
}

} // namespace

class AISmartLogRepo::Impl
{
public:
    std::unique_ptr<pqxx::connection> conn;
    std::string init_error;
    Impl()
    {
        conn = GetPgConn(init_error);
    }

    bool Ready() const
    {
        return conn != nullptr && conn->is_open();
    }
};

AISmartLogRepo::AISmartLogRepo()
    : _impl(std::make_unique<Impl>())
{
}

AISmartLogRepo::~AISmartLogRepo() = default;

void AISmartLogRepo::LogSmartUsage(int32_t uid,
                                   const std::string& feature_type,
                                   int input_tokens,
                                   int output_tokens,
                                   const std::string& model_name)
{
    int64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (!_impl->Ready())
    {
        memolog::LogError("ai_smart_log.failed",
                          "pg_connection_unavailable",
                          {{"uid", std::to_string(uid)}, {"feature_type", feature_type}, {"error", _impl->init_error}});
        return;
    }
    pqxx::work tx(*_impl->conn);
    const auto inserted = tx.exec_params(
        R"(INSERT INTO ai_smart_log (uid, feature_type, input_tokens, output_tokens, model_name, created_at)
           VALUES ($1, $2, $3, $4, $5, $6))",
        uid,
        feature_type,
        input_tokens,
        output_tokens,
        model_name,
        now);
    if (!inserted.ok() || !tx.commit())
    {
        memolog::LogError("ai_smart_log.failed",
                          "pg_insert_error",
                          {{"uid", std::to_string(uid)},
                           {"feature_type", feature_type},
                           {"error", inserted.ok() ? tx.error_message() : inserted.error_message()}});
    }
}
