#include "db/AISmartLogRepo.hpp"
#include "ConfigMgr.hpp"
#include "db/PqxxCompat.hpp"
#include "logging/Logger.hpp"
#include <pqxx/pqxx>
#include <pqxx/transaction.hxx>
#include <chrono>
#include <sstream>

import memochat.ai.repository_algorithms;

namespace
{
namespace repository_modules = memochat::ai::repository::modules;

pqxx::connection* GetPgConn()
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
    pqxx::work tx(*conn);
    tx.exec0("SET search_path TO " + tx.quote_name(schema) + ",public");
    tx.commit();
    return conn.release();
}

} // namespace

class AISmartLogRepo::Impl
{
public:
    std::unique_ptr<pqxx::connection> conn;
    Impl()
    {
        conn.reset(GetPgConn());
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
    try
    {
        pqxx::work tx(*_impl->conn);
        tx.exec_params(
            R"(INSERT INTO ai_smart_log (uid, feature_type, input_tokens, output_tokens, model_name, created_at)
               VALUES ($1, $2, $3, $4, $5, $6))",
            uid,
            feature_type,
            input_tokens,
            output_tokens,
            model_name,
            now);
        tx.commit();
    }
    catch (const std::exception& e)
    {
        memolog::LogError("ai_smart_log.failed",
                          "pg_insert_error",
                          {{"uid", std::to_string(uid)}, {"feature_type", feature_type}, {"error", e.what()}});
    }
}
