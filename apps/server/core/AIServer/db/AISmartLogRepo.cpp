#include "db/AISmartLogRepo.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include <pqxx/pqxx>
#include <pqxx/transaction.hxx>
#include <chrono>
#include <sstream>

using namespace std::chrono;

namespace {

pqxx::connection* GetPgConn() {
    auto& cfg = ConfigMgr::Inst();
    std::ostringstream conn_str;
    conn_str << "host=" << cfg["Postgres"]["Host"]
             << " port=" << cfg["Postgres"]["Port"]
             << " user=" << cfg["Postgres"]["User"]
             << " password=" << cfg["Postgres"]["Passwd"]
             << " dbname=" << cfg["Postgres"]["Database"]
             << " options=-c search_path=" << cfg["Postgres"]["Schema"];
    return new pqxx::connection(conn_str.str());
}

} // namespace

class AISmartLogRepo::Impl {
public:
    std::unique_ptr<pqxx::connection> conn;
    Impl() { conn.reset(GetPgConn()); }
};

AISmartLogRepo::AISmartLogRepo()
    : _impl(std::make_unique<Impl>()) {}

AISmartLogRepo::~AISmartLogRepo() = default;

void AISmartLogRepo::LogSmartUsage(int32_t uid, const std::string& feature_type,
                                   int input_tokens, int output_tokens,
                                   const std::string& model_name) {
    int64_t now = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
    try {
        pqxx::work tx(*_impl->conn);
        tx.exec_params(
            R"(INSERT INTO ai_smart_log (uid, feature_type, input_tokens, output_tokens, model_name, created_at)
               VALUES ($1, $2, $3, $4, $5, $6))",
            uid, feature_type, input_tokens, output_tokens, model_name, now);
        tx.commit();
    } catch (const std::exception& e) {
        memolog::LogError("ai_smart_log.failed",
            "pg_insert_error",
            {{"uid", std::to_string(uid)}, {"feature_type", feature_type},
             {"error", e.what()}});
    }
}
