#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <cctype>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace {
std::string DecodeLegacyXorPwd(const std::string& input) {
	unsigned int xor_code = static_cast<unsigned int>(input.size() % 255);
	std::string decoded = input;
	for (size_t i = 0; i < decoded.size(); ++i) {
		decoded[i] = static_cast<char>(static_cast<unsigned char>(decoded[i]) ^ xor_code);
	}
	return decoded;
}

bool IsValidUserPublicId(const std::string& user_id) {
	if (user_id.size() != 10 || user_id[0] != 'u') {
		return false;
	}
	if (user_id[1] < '1' || user_id[1] > '9') {
		return false;
	}
	for (size_t i = 2; i < user_id.size(); ++i) {
		if (!std::isdigit(static_cast<unsigned char>(user_id[i]))) {
			return false;
		}
	}
	return true;
}
}

MysqlDao::MysqlDao()
{
	auto & cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	std::string mysql_url = host;
	if (mysql_url.find("://") == std::string::npos) {
		mysql_url = "tcp://" + mysql_url;
	}
	pool_.reset(new MySqlPool(mysql_url + ":" + port, user, pwd, schema, 5));
	if (!EnsureUserPublicIdSchemaAndBackfill()) {
		throw std::runtime_error("failed to ensure/backfill user.user_id");
	}
	if (!EnsureMediaAssetSchema()) {
		throw std::runtime_error("failed to ensure chat_media_asset schema");
	}
	if (!EnsureCallSessionSchema()) {
		throw std::runtime_error("failed to ensure chat_call_session schema");
	}
	WarmupAuthQueries();
}

MysqlDao::~MysqlDao(){
	pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));

		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);




		stmt->execute();


	   unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
	  unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
	  if (res->next()) {
	       int result = res->getInt("result");
	      cout << "Result: " << result << endl;
		  pool_->returnConnection(std::move(con));
		  return result;
	  }
	  pool_->returnConnection(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

int MysqlDao::RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, 
	const std::string& icon)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con] {
		pool_->returnConnection(std::move(con));
	});

	try {

		con->_con->setAutoCommit(false);



		std::unique_ptr<sql::PreparedStatement> pstmt_email(con->_con->prepareStatement("SELECT 1 FROM user WHERE email = ?"));


		pstmt_email->setString(1, email);


		std::unique_ptr<sql::ResultSet> res_email(pstmt_email->executeQuery());

		auto email_exist = res_email->next();
		if (email_exist) {
			con->_con->rollback();
			std::cout << "email " << email << " exist";
			return 0;
		}


		std::unique_ptr<sql::PreparedStatement> pstmt_name(con->_con->prepareStatement("SELECT 1 FROM user WHERE name = ?"));


		pstmt_name->setString(1, name);


		std::unique_ptr<sql::ResultSet> res_name(pstmt_name->executeQuery());

		auto name_exist = res_name->next();
		if (name_exist) {
			con->_con->rollback();
			std::cout << "name " << name << " exist";
			return 0;
		}


		std::unique_ptr<sql::PreparedStatement> pstmt_upid(con->_con->prepareStatement("UPDATE user_id SET id = id + 1"));


		pstmt_upid->executeUpdate();


		std::unique_ptr<sql::PreparedStatement> pstmt_uid(con->_con->prepareStatement("SELECT id FROM user_id"));
		std::unique_ptr<sql::ResultSet> res_uid(pstmt_uid->executeQuery());
		int newId = 0;

		if (res_uid->next()) {
			newId = res_uid->getInt("id");
		}
		else {
			std::cout << "select id from user_id failed" << std::endl;
			con->_con->rollback();
			return -1;
		}

		std::string user_public_id;
		for (int i = 0; i < 20; ++i) {
			user_public_id = GenerateRandomUserPublicId();
			std::unique_ptr<sql::PreparedStatement> check_user_id(
				con->_con->prepareStatement("SELECT 1 FROM user WHERE user_id = ? LIMIT 1"));
			check_user_id->setString(1, user_public_id);
			std::unique_ptr<sql::ResultSet> check_user_id_res(check_user_id->executeQuery());
			if (!check_user_id_res->next()) {
				break;
			}
			user_public_id.clear();
		}

		if (user_public_id.empty()) {
			con->_con->rollback();
			std::cout << "generate user_public_id failed" << std::endl;
			return -1;
		}


		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement("INSERT INTO user (uid, name, email, pwd, nick, icon, user_id) "
			"VALUES (?, ?, ?, ?, ?, ?, ?)"));
		pstmt_insert->setInt(1,newId);
		pstmt_insert->setString(2, name);
		pstmt_insert->setString(3, email);
		pstmt_insert->setString(4, pwd);
		pstmt_insert->setString(5, name);
		pstmt_insert->setString(6, icon);
		pstmt_insert->setString(7, user_public_id);

		pstmt_insert->executeUpdate();

		con->_con->commit();
		std::cout << "newuser insert into user success" << std::endl;
		return newId;
	}
	catch (sql::SQLException& e) {

		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}


		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));


		pstmt->setString(1, name);


		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());


		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				pool_->returnConnection(std::move(con));
				return false;
			}
			pool_->returnConnection(std::move(con));
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}


		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));


		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);


		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("UPDATE user SET nick = ?, `desc` = ?, icon = ? WHERE uid = ?"));
		pstmt->setString(1, nick);
		pstmt->setString(2, desc);
		pstmt->setString(3, icon);
		pstmt->setInt(4, uid);

		int updateCount = pstmt->executeUpdate();
		if (updateCount > 0) {
			pool_->returnConnection(std::move(con));
			return true;
		}

		// MySQL may report 0 rows affected when values are unchanged.
		std::unique_ptr<sql::PreparedStatement> checkStmt(
			con->_con->prepareStatement("SELECT 1 FROM user WHERE uid = ? LIMIT 1"));
		checkStmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
		const bool uidExists = checkRes->next();
		pool_->returnConnection(std::move(con));
		return uidExists;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT uid, name, email, pwd, user_id, nick, icon, `desc`, sex "
			"FROM user WHERE email = ? LIMIT 1"));
		pstmt->setString(1, email);


		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		bool matched = false;

		while (res->next()) {
			origin_pwd = res->getString("pwd");
		userInfo.name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.user_id = res->getString("user_id");
		userInfo.nick = res->isNull("nick") ? "" : res->getString("nick");
		userInfo.icon = res->isNull("icon") ? "" : res->getString("icon");
		userInfo.desc = res->isNull("desc") ? "" : res->getString("desc");
		userInfo.sex = res->isNull("sex") ? 0 : res->getInt("sex");

			std::cout << "Password: " << origin_pwd << std::endl;
			matched = true;
			break;
		}

		if (!matched) {
			return false;
		}

		if (pwd != origin_pwd) {
			const auto decoded_pwd = DecodeLegacyXorPwd(pwd);
			if (decoded_pwd != origin_pwd) {
				return false;
			}
		}
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

std::string MysqlDao::GetUserPublicId(int uid) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return "";
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("SELECT user_id FROM user WHERE uid = ? LIMIT 1"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return "";
		}
		return res->getString("user_id");
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return "";
	}
}

void MysqlDao::WarmupAuthQueries() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT uid, name, email, pwd, user_id, nick, icon, `desc`, sex "
			"FROM user WHERE email = ? LIMIT 1"));
		pstmt->setString(1, "__memochat_warmup__@invalid.local");
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		(void)res;
	}
	catch (sql::SQLException& e) {
		std::cerr << "WarmupAuthQueries SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
	}
}

bool MysqlDao::GetCallUserProfile(int uid, CallUserProfile& profile) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT uid, user_id, name, nick, icon FROM user WHERE uid = ? LIMIT 1"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		profile.uid = res->getInt("uid");
		profile.user_id = res->isNull("user_id") ? "" : res->getString("user_id");
		profile.name = res->isNull("name") ? "" : res->getString("name");
		profile.nick = res->isNull("nick") ? "" : res->getString("nick");
		profile.icon = res->isNull("icon") ? "" : res->getString("icon");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "GetCallUserProfile SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::IsFriend(int uid, int peer_uid) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("SELECT 1 FROM friend WHERE self_id = ? AND friend_id = ? LIMIT 1"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, peer_uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		return res->next();
	}
	catch (sql::SQLException& e) {
		std::cerr << "IsFriend SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpsertCallSession(const CallSessionInfo& session) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"INSERT INTO chat_call_session("
				"call_id, room_name, call_type, caller_uid, callee_uid, state, "
				"started_at_ms, accepted_at_ms, ended_at_ms, expires_at_ms, duration_sec, reason, trace_id, updated_at_ms"
				") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
				"ON DUPLICATE KEY UPDATE "
				"room_name=VALUES(room_name), call_type=VALUES(call_type), caller_uid=VALUES(caller_uid), "
				"callee_uid=VALUES(callee_uid), state=VALUES(state), started_at_ms=VALUES(started_at_ms), "
				"accepted_at_ms=VALUES(accepted_at_ms), ended_at_ms=VALUES(ended_at_ms), "
				"expires_at_ms=VALUES(expires_at_ms), duration_sec=VALUES(duration_sec), "
				"reason=VALUES(reason), trace_id=VALUES(trace_id), updated_at_ms=VALUES(updated_at_ms)"));
		pstmt->setString(1, session.call_id);
		pstmt->setString(2, session.room_name);
		pstmt->setString(3, session.call_type);
		pstmt->setInt(4, session.caller_uid);
		pstmt->setInt(5, session.callee_uid);
		pstmt->setString(6, session.state);
		pstmt->setInt64(7, session.started_at_ms);
		pstmt->setInt64(8, session.accepted_at_ms);
		pstmt->setInt64(9, session.ended_at_ms);
		pstmt->setInt64(10, session.expires_at_ms);
		pstmt->setInt(11, session.duration_sec);
		pstmt->setString(12, session.reason);
		pstmt->setString(13, session.trace_id);
		pstmt->setInt64(14, session.updated_at_ms);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "UpsertCallSession SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetCallSession(const std::string& call_id, CallSessionInfo& session) {
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT call_id, room_name, call_type, caller_uid, callee_uid, state, "
				"started_at_ms, accepted_at_ms, ended_at_ms, expires_at_ms, duration_sec, reason, trace_id, updated_at_ms "
				"FROM chat_call_session WHERE call_id = ? LIMIT 1"));
		pstmt->setString(1, call_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		session.call_id = res->isNull("call_id") ? "" : res->getString("call_id");
		session.room_name = res->isNull("room_name") ? "" : res->getString("room_name");
		session.call_type = res->isNull("call_type") ? "" : res->getString("call_type");
		session.caller_uid = res->isNull("caller_uid") ? 0 : res->getInt("caller_uid");
		session.callee_uid = res->isNull("callee_uid") ? 0 : res->getInt("callee_uid");
		session.state = res->isNull("state") ? "" : res->getString("state");
		session.started_at_ms = res->isNull("started_at_ms") ? 0 : res->getInt64("started_at_ms");
		session.accepted_at_ms = res->isNull("accepted_at_ms") ? 0 : res->getInt64("accepted_at_ms");
		session.ended_at_ms = res->isNull("ended_at_ms") ? 0 : res->getInt64("ended_at_ms");
		session.expires_at_ms = res->isNull("expires_at_ms") ? 0 : res->getInt64("expires_at_ms");
		session.duration_sec = res->isNull("duration_sec") ? 0 : res->getInt("duration_sec");
		session.reason = res->isNull("reason") ? "" : res->getString("reason");
		session.trace_id = res->isNull("trace_id") ? "" : res->getString("trace_id");
		session.updated_at_ms = res->isNull("updated_at_ms") ? 0 : res->getInt64("updated_at_ms");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "GetCallSession SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::InsertMediaAsset(const MediaAssetInfo& asset) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"INSERT INTO chat_media_asset("
				"media_key, owner_uid, media_type, origin_file_name, mime, size_bytes, "
				"storage_provider, storage_path, created_at_ms, deleted_at_ms, status"
				") VALUES(?,?,?,?,?,?,?,?,?,?,?)"));
		pstmt->setString(1, asset.media_key);
		pstmt->setInt(2, asset.owner_uid);
		pstmt->setString(3, asset.media_type);
		pstmt->setString(4, asset.origin_file_name);
		pstmt->setString(5, asset.mime);
		pstmt->setInt64(6, asset.size_bytes);
		pstmt->setString(7, asset.storage_provider);
		pstmt->setString(8, asset.storage_path);
		pstmt->setInt64(9, asset.created_at_ms);
		pstmt->setInt64(10, asset.deleted_at_ms);
		pstmt->setInt(11, asset.status);
		const int update = pstmt->executeUpdate();
		return update > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "InsertMediaAsset SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset) {
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT media_id, media_key, owner_uid, media_type, origin_file_name, mime, size_bytes, "
				"storage_provider, storage_path, created_at_ms, deleted_at_ms, status "
				"FROM chat_media_asset WHERE media_key = ? LIMIT 1"));
		pstmt->setString(1, media_key);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}

		asset.media_id = res->getInt64("media_id");
		asset.media_key = res->getString("media_key");
		asset.owner_uid = res->getInt("owner_uid");
		asset.media_type = res->isNull("media_type") ? "" : res->getString("media_type");
		asset.origin_file_name = res->isNull("origin_file_name") ? "" : res->getString("origin_file_name");
		asset.mime = res->isNull("mime") ? "" : res->getString("mime");
		asset.size_bytes = res->isNull("size_bytes") ? 0 : res->getInt64("size_bytes");
		asset.storage_provider = res->isNull("storage_provider") ? "" : res->getString("storage_provider");
		asset.storage_path = res->isNull("storage_path") ? "" : res->getString("storage_path");
		asset.created_at_ms = res->isNull("created_at_ms") ? 0 : res->getInt64("created_at_ms");
		asset.deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
		asset.status = res->isNull("status") ? 0 : res->getInt("status");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "GetMediaAssetByKey SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

std::string MysqlDao::GenerateRandomUserPublicId() {
	static thread_local std::mt19937_64 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(100000000, 999999999);
	return "u" + std::to_string(dist(rng));
}

bool MysqlDao::EnsureUserPublicIdSchemaAndBackfill() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		try {
			stmt->execute("ALTER TABLE user ADD COLUMN user_id VARCHAR(10) NULL");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("CREATE UNIQUE INDEX uk_user_user_id ON user(user_id)");
		}
		catch (sql::SQLException&) {
		}

		std::unique_ptr<sql::PreparedStatement> query(
			con->_con->prepareStatement("SELECT uid, user_id FROM user ORDER BY uid ASC"));
		std::unique_ptr<sql::ResultSet> rows(query->executeQuery());

		struct UserRow {
			int uid = 0;
			std::string user_id;
		};
		std::vector<UserRow> all_rows;
		std::unordered_set<std::string> used_user_ids;
		int duplicated_marked = 0;

		while (rows->next()) {
			UserRow row;
			row.uid = rows->getInt("uid");
			row.user_id = rows->isNull("user_id") ? "" : rows->getString("user_id");
			if (IsValidUserPublicId(row.user_id)) {
				if (used_user_ids.find(row.user_id) == used_user_ids.end()) {
					used_user_ids.insert(row.user_id);
				}
				else {
					row.user_id.clear();
					++duplicated_marked;
				}
			}
			else {
				row.user_id.clear();
			}
			all_rows.push_back(row);
		}

		int backfilled = 0;
		int retries = 0;
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> update_stmt(
			con->_con->prepareStatement("UPDATE user SET user_id = ? WHERE uid = ?"));

		for (auto& row : all_rows) {
			if (!row.user_id.empty()) {
				continue;
			}

			std::string new_user_id;
			for (int i = 0; i < 20; ++i) {
				const std::string candidate = GenerateRandomUserPublicId();
				if (used_user_ids.find(candidate) == used_user_ids.end()) {
					new_user_id = candidate;
					break;
				}
				++retries;
			}

			if (new_user_id.empty()) {
				con->_con->rollback();
				return false;
			}

			update_stmt->setString(1, new_user_id);
			update_stmt->setInt(2, row.uid);
			update_stmt->executeUpdate();
			used_user_ids.insert(new_user_id);
			++backfilled;
		}

		con->_con->commit();
		std::cout << "[GateServer] user_id backfill done. total=" << all_rows.size()
			<< " backfilled=" << backfilled
			<< " duplicate_fixed=" << duplicated_marked
			<< " retries=" << retries << std::endl;
		return true;
	}
	catch (sql::SQLException& e) {
		if (con && con->_con) {
			try {
				con->_con->rollback();
			}
			catch (...) {
			}
		}
		std::cerr << "EnsureUserPublicIdSchemaAndBackfill SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::EnsureMediaAssetSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute(
			"CREATE TABLE IF NOT EXISTS chat_media_asset ("
			"media_id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"media_key VARCHAR(64) NOT NULL,"
			"owner_uid INT NOT NULL,"
			"media_type VARCHAR(16) NOT NULL,"
			"origin_file_name VARCHAR(255) NOT NULL,"
			"mime VARCHAR(127) DEFAULT '',"
			"size_bytes BIGINT NOT NULL DEFAULT 0,"
			"storage_provider VARCHAR(32) NOT NULL DEFAULT 'local',"
			"storage_path VARCHAR(1024) NOT NULL,"
			"created_at_ms BIGINT NOT NULL,"
			"deleted_at_ms BIGINT NOT NULL DEFAULT 0,"
			"status TINYINT NOT NULL DEFAULT 1,"
			"UNIQUE KEY uk_media_key(media_key),"
			"KEY idx_owner_created(owner_uid, created_at_ms)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureMediaAssetSchema SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::EnsureCallSessionSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute(
			"CREATE TABLE IF NOT EXISTS chat_call_session ("
			"call_id VARCHAR(64) PRIMARY KEY,"
			"room_name VARCHAR(128) NOT NULL,"
			"call_type VARCHAR(16) NOT NULL,"
			"caller_uid INT NOT NULL,"
			"callee_uid INT NOT NULL,"
			"state VARCHAR(32) NOT NULL,"
			"started_at_ms BIGINT NOT NULL,"
			"accepted_at_ms BIGINT NOT NULL DEFAULT 0,"
			"ended_at_ms BIGINT NOT NULL DEFAULT 0,"
			"expires_at_ms BIGINT NOT NULL DEFAULT 0,"
			"duration_sec INT NOT NULL DEFAULT 0,"
			"reason VARCHAR(64) NOT NULL DEFAULT '',"
			"trace_id VARCHAR(64) NOT NULL DEFAULT '',"
			"updated_at_ms BIGINT NOT NULL DEFAULT 0,"
			"KEY idx_call_caller(caller_uid, started_at_ms),"
			"KEY idx_call_callee(callee_uid, started_at_ms),"
			"KEY idx_call_state(state, updated_at_ms)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureCallSessionSchema SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::TestProcedure(const std::string& email, int& uid, string& name) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		Defer defer([this, &con]() {
			pool_->returnConnection(std::move(con));
			});

		unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL test_procedure(?,@userId,@userName)"));

		stmt->setString(1, email);
		



		stmt->execute();


		unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @userId AS uid"));
		if (!(res->next())) {
			return false;
		}
		
		uid = res->getInt("uid");
		cout << "uid: " << uid << endl;
		
		stmtResult.reset(con->_con->createStatement());
		res.reset(stmtResult->executeQuery("SELECT @userName AS name"));
		if (!(res->next())) {
			return false;
		}
		
		name = res->getString("name");
		cout << "name: " << name << endl;
		return true;

	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
