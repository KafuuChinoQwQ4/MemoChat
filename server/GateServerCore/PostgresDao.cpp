#include "PostgresDao.h"
#include "ConfigMgr.h"
#include <cctype>
#include <random>

namespace {
std::string BuildConnectionString() {
	auto& cfg = ConfigMgr::Inst();
	const auto host = cfg["Postgres"]["Host"];
	const auto port = cfg["Postgres"]["Port"];
	const auto pwd = cfg["Postgres"]["Passwd"];
	const auto database = cfg["Postgres"]["Database"];
	const auto schema = cfg["Postgres"]["Schema"];
	const auto user = cfg["Postgres"]["User"];
	const auto sslmode = cfg["Postgres"]["SslMode"];
	std::string conn = "host=" + host +
		" port=" + port +
		" user=" + user +
		" password=" + pwd +
		" dbname=" + database +
		" sslmode=" + (sslmode.empty() ? "disable" : sslmode) +
		" options=-csearch_path=" + schema + ",public";
	return conn;
}

std::string DecodeLegacyXorPwd(const std::string& input) {
	unsigned int xor_code = static_cast<unsigned int>(input.size() % 255);
	std::string decoded = input;
	for (size_t i = 0; i < decoded.size(); ++i) {
		decoded[i] = static_cast<char>(static_cast<unsigned char>(decoded[i]) ^ xor_code);
	}
	return decoded;
}
}

PostgresDao::PostgresDao()
{
	pool_.reset(new PostgresPool(BuildConnectionString(), 60));
	WarmupAuthQueries();
}

PostgresDao::~PostgresDao() {
	pool_->Close();
}

int PostgresDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	return RegUserTransaction(name, email, pwd, ":/res/head_1.jpg");
}

int PostgresDao::RegUserTransaction(
	const std::string& name,
	const std::string& email,
	const std::string& pwd,
	const std::string& icon) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return -1;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::work txn(*con->_con);
		const auto exists = txn.exec_params(
			"SELECT 1 FROM \"user\" WHERE email = $1 OR name = $2 LIMIT 1",
			email,
			name);
		if (!exists.empty()) {
			txn.commit();
			std::cout << "user " << name << " or email " << email << " exist" << std::endl;
			return 0;
		}

		const int new_id = txn.exec1("SELECT COALESCE(MAX(id), 1000) + 1 FROM user_id")[0].as<int>();
		txn.exec0("DELETE FROM user_id");
		txn.exec_params0("INSERT INTO user_id(id) VALUES ($1)", new_id);

		std::string user_public_id;
		for (int i = 0; i < 20; ++i) {
			user_public_id = GenerateRandomUserPublicId();
			const auto rows = txn.exec_params(
				"SELECT 1 FROM \"user\" WHERE user_id = $1 LIMIT 1",
				user_public_id);
			if (rows.empty()) {
				break;
			}
			user_public_id.clear();
		}

		if (user_public_id.empty()) {
			std::cout << "generate user_public_id failed" << std::endl;
			return -1;
		}

		const std::string pwd_stored = DecodeLegacyXorPwd(pwd);
		txn.exec_params0(
			"INSERT INTO \"user\"(uid, name, email, pwd, nick, icon, user_id) "
			"VALUES ($1, $2, $3, $4, $5, $6, $7)",
			new_id,
			name,
			email,
			pwd_stored,
			name,
			icon,
			user_public_id);
		txn.commit();
		std::cout << "newuser insert into user success" << std::endl;
		return new_id;
	}
	catch (const std::exception& e) {
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return -1;
	}
}

bool PostgresDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT email FROM \"user\" WHERE name = $1 LIMIT 1",
			name);
		if (rows.empty()) {
			return false;
		}
		const auto stored_email = rows[0]["email"].c_str() ? rows[0]["email"].c_str() : "";
		return stored_email == email;
	}
	catch (const std::exception& e) {
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::work txn(*con->_con);
		const auto result = txn.exec_params(
			"UPDATE \"user\" SET pwd = $1 WHERE name = $2",
			newpwd,
			name);
		txn.commit();
		std::cout << "Updated rows: " << result.affected_rows() << std::endl;
		return result.affected_rows() > 0;
	}
	catch (const std::exception& e) {
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::work txn(*con->_con);
		const auto result = txn.exec_params(
			"UPDATE \"user\" SET nick = $1, \"desc\" = $2, icon = $3 WHERE uid = $4",
			nick,
			desc,
			icon,
			uid);
		if (result.affected_rows() > 0) {
			txn.commit();
			return true;
		}

		const auto rows = txn.exec_params(
			"SELECT 1 FROM \"user\" WHERE uid = $1 LIMIT 1",
			uid);
		txn.commit();
		return !rows.empty();
	}
	catch (const std::exception& e) {
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT uid, name, email, pwd, user_id, nick, icon, \"desc\", sex "
			"FROM \"user\" WHERE email = $1 LIMIT 1",
			email);
		if (rows.empty()) {
			return false;
		}

		const auto& row = rows[0];
		const std::string origin_pwd = row["pwd"].c_str() ? row["pwd"].c_str() : "";
		const std::string decoded_pwd = DecodeLegacyXorPwd(origin_pwd);
		if (pwd != origin_pwd && pwd != decoded_pwd) {
			return false;
		}

		userInfo.name = row["name"].c_str() ? row["name"].c_str() : "";
		userInfo.email = row["email"].c_str() ? row["email"].c_str() : "";
		userInfo.uid = row["uid"].as<int>();
		userInfo.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
		userInfo.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
		userInfo.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
		userInfo.desc = row["desc"].is_null() ? "" : row["desc"].c_str();
		userInfo.sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
		userInfo.pwd = decoded_pwd;
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

std::string PostgresDao::GetUserPublicId(int uid) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return "";
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT user_id FROM \"user\" WHERE uid = $1 LIMIT 1",
			uid);
		if (rows.empty() || rows[0]["user_id"].is_null()) {
			return "";
		}
		return rows[0]["user_id"].c_str();
	}
	catch (const std::exception& e) {
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return "";
	}
}

void PostgresDao::WarmupAuthQueries() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		txn.exec_params(
			"SELECT uid, name, email, pwd, user_id, nick, icon, \"desc\", sex "
			"FROM \"user\" WHERE email = $1 LIMIT 1",
			"__memochat_warmup__@invalid.local");
		txn.commit();
	}
	catch (const std::exception& e) {
		std::cerr << "[DIAG WarmupAuthQueries] PostgreSQL exception: " << e.what() << "\n" << std::flush;
	}
}

bool PostgresDao::GetCallUserProfile(int uid, CallUserProfile& profile) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT uid, user_id, name, nick, icon FROM \"user\" WHERE uid = $1 LIMIT 1",
			uid);
		if (rows.empty()) {
			return false;
		}

		const auto& row = rows[0];
		profile.uid = row["uid"].as<int>();
		profile.user_id = row["user_id"].is_null() ? "" : row["user_id"].c_str();
		profile.name = row["name"].is_null() ? "" : row["name"].c_str();
		profile.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
		profile.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "GetCallUserProfile PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::IsFriend(int uid, int peer_uid) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT 1 FROM friend WHERE self_id = $1 AND friend_id = $2 LIMIT 1",
			uid,
			peer_uid);
		return !rows.empty();
	}
	catch (const std::exception& e) {
		std::cerr << "IsFriend PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::UpsertCallSession(const CallSessionInfo& session) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
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
		txn.commit();
		return result.affected_rows() > 0;
	}
	catch (const std::exception& e) {
		std::cerr << "UpsertCallSession PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::GetCallSession(const std::string& call_id, CallSessionInfo& session) {
	if (call_id.empty()) {
		return false;
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT call_id, room_name, call_type, caller_uid, callee_uid, state, "
			"started_at_ms, accepted_at_ms, ended_at_ms, expires_at_ms, duration_sec, reason, trace_id, updated_at_ms "
			"FROM chat_call_session WHERE call_id = $1 LIMIT 1",
			call_id);
		if (rows.empty()) {
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
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "GetCallSession PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::InsertMediaAsset(const MediaAssetInfo& asset) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::work txn(*con->_con);
		const auto result = txn.exec_params(
			"INSERT INTO chat_media_asset("
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
		txn.commit();
		return result.affected_rows() > 0;
	}
	catch (const std::exception& e) {
		std::cerr << "InsertMediaAsset PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset) {
	if (media_key.empty()) {
		return false;
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT media_id, media_key, owner_uid, media_type, origin_file_name, mime, size_bytes, "
			"storage_provider, storage_path, created_at_ms, deleted_at_ms, status "
			"FROM chat_media_asset WHERE media_key = $1 LIMIT 1",
			media_key);
		if (rows.empty()) {
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
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "GetMediaAssetByKey PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

std::string PostgresDao::GenerateRandomUserPublicId() {
	static thread_local std::mt19937_64 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(100000000, 999999999);
	return "u" + std::to_string(dist(rng));
}

bool PostgresDao::GetUserInfo(int uid, UserInfo& user_info) {
	if (uid <= 0) {
		return false;
	}
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec(
			"SELECT uid, name, email, nick, icon, \"desc\", sex "
			"FROM memo.user_base WHERE uid = " + std::to_string(uid) + " LIMIT 1");
		if (rows.empty()) {
			return false;
		}
		const auto& row = rows[0];
		user_info.uid = row["uid"].as<int>();
		user_info.name = row["name"].is_null() ? "" : row["name"].c_str();
		user_info.email = row["email"].is_null() ? "" : row["email"].c_str();
		user_info.nick = row["nick"].is_null() ? "" : row["nick"].c_str();
		user_info.icon = row["icon"].is_null() ? "" : row["icon"].c_str();
		user_info.desc = row["desc"].is_null() ? "" : row["desc"].c_str();
		user_info.sex = row["sex"].is_null() ? 0 : row["sex"].as<int>();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "GetUserInfo PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::TestProcedure(const std::string& email, int& uid, string& name) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		pqxx::read_transaction txn(*con->_con);
		const auto rows = txn.exec_params(
			"SELECT uid, name FROM \"user\" WHERE email = $1 LIMIT 1",
			email);
		if (rows.empty()) {
			return false;
		}
		uid = rows[0]["uid"].as<int>();
		name = rows[0]["name"].c_str() ? rows[0]["name"].c_str() : "";
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "TestProcedure PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}
