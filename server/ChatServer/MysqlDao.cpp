#include "MysqlDao.h"
#include "ConfigMgr.h"
#include "MySqlPool.h"
#include <jdbc/cppconn/prepared_statement.h>
#include <set>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <stdexcept>
#include <limits>

namespace {
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

bool IsValidGroupCode(const std::string& group_code) {
	if (group_code.size() != 10 || group_code[0] != 'g') {
		return false;
	}
	if (group_code[1] < '1' || group_code[1] > '9') {
		return false;
	}
	for (size_t i = 2; i < group_code.size(); ++i) {
		if (!std::isdigit(static_cast<unsigned char>(group_code[i]))) {
			return false;
		}
	}
	return true;
}

constexpr int64_t kPermChangeGroupInfo = 1LL << 0;
constexpr int64_t kPermDeleteMessages = 1LL << 1;
constexpr int64_t kPermInviteUsers = 1LL << 2;
constexpr int64_t kPermManageAdmins = 1LL << 3;
constexpr int64_t kPermPinMessages = 1LL << 4;
constexpr int64_t kPermBanUsers = 1LL << 5;
constexpr int64_t kPermManageTopics = 1LL << 6;
constexpr int64_t kDefaultAdminPermBits =
	kPermChangeGroupInfo | kPermDeleteMessages | kPermInviteUsers | kPermPinMessages | kPermBanUsers;
constexpr int64_t kOwnerPermBits =
	kDefaultAdminPermBits | kPermManageAdmins | kPermManageTopics;

std::string BuildPreviewText(const std::string& msg_type, const std::string& content) {
	if (msg_type == "image" || content.rfind("__memochat_img__:", 0) == 0) {
		return "[图片]";
	}
	if (msg_type == "file" || content.rfind("__memochat_file__:", 0) == 0) {
		return "[文件]";
	}
	if (msg_type == "call" || content.rfind("__memochat_call__:", 0) == 0) {
		return "[通话邀请]";
	}
	if (content.rfind("__memochat_reply__:", 0) == 0) {
		return "[回复]";
	}
	if (content.size() <= 80) {
		return content;
	}
	return content.substr(0, 80);
}

void ExecuteIgnoreSql(sql::Statement* stmt, const std::string& sql_text) {
	if (stmt == nullptr) {
		return;
	}
	try {
		stmt->execute(sql_text);
	}
	catch (const sql::SQLException&) {
	}
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
	pool_.reset(new MySqlPool(host+":"+port, user, pwd,schema, 5));
	try {
		auto con = pool_->getConnection();
		if (con != nullptr) {
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->execute("CREATE TABLE IF NOT EXISTS friend_apply_tag ("
				"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
				"from_uid INT NOT NULL,"
				"to_uid INT NOT NULL,"
				"tag VARCHAR(64) NOT NULL,"
				"UNIQUE KEY uq_apply_tag(from_uid, to_uid, tag)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS friend_tag ("
				"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
				"self_id INT NOT NULL,"
				"friend_id INT NOT NULL,"
				"tag VARCHAR(64) NOT NULL,"
				"UNIQUE KEY uq_friend_tag(self_id, friend_id, tag)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS chat_group ("
				"group_id BIGINT AUTO_INCREMENT PRIMARY KEY,"
				"group_code VARCHAR(10) NULL,"
				"name VARCHAR(64) NOT NULL,"
				"icon VARCHAR(512) NULL,"
				"owner_uid INT NOT NULL,"
				"announcement TEXT,"
				"member_limit INT NOT NULL DEFAULT 200,"
				"is_all_muted TINYINT NOT NULL DEFAULT 0,"
				"status TINYINT NOT NULL DEFAULT 1,"
				"created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
				"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
				"UNIQUE KEY uk_group_code(group_code),"
				"KEY idx_owner_uid(owner_uid),"
				"KEY idx_status(status)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_member ("
				"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
				"group_id BIGINT NOT NULL,"
				"uid INT NOT NULL,"
				"role TINYINT NOT NULL DEFAULT 1,"
				"mute_until BIGINT NOT NULL DEFAULT 0,"
				"join_source TINYINT NOT NULL DEFAULT 0,"
				"status TINYINT NOT NULL DEFAULT 1,"
				"joined_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
				"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
				"UNIQUE KEY uq_group_user(group_id, uid),"
				"KEY idx_uid_status(uid, status),"
				"KEY idx_group_status(group_id, status)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_apply ("
				"apply_id BIGINT AUTO_INCREMENT PRIMARY KEY,"
				"group_id BIGINT NOT NULL,"
				"applicant_uid INT NOT NULL,"
				"inviter_uid INT DEFAULT 0,"
				"type TINYINT NOT NULL DEFAULT 1,"
				"status TINYINT NOT NULL DEFAULT 0,"
				"reason VARCHAR(128) DEFAULT '',"
				"reviewer_uid INT DEFAULT 0,"
				"created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
				"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
				"KEY idx_group_status(group_id, status, created_at),"
				"KEY idx_applicant(group_id, applicant_uid, status),"
				"KEY idx_inviter(group_id, inviter_uid, status)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_msg ("
				"msg_id VARCHAR(64) PRIMARY KEY,"
				"server_msg_id BIGINT NOT NULL AUTO_INCREMENT UNIQUE,"
				"group_id BIGINT NOT NULL,"
				"group_seq BIGINT NOT NULL DEFAULT 0,"
				"from_uid INT NOT NULL,"
				"msg_type VARCHAR(16) NOT NULL,"
				"content TEXT NOT NULL,"
				"mentions_json TEXT,"
				"reply_to_server_msg_id BIGINT NOT NULL DEFAULT 0,"
				"forward_meta_json TEXT,"
				"edited_at_ms BIGINT NOT NULL DEFAULT 0,"
				"deleted_at_ms BIGINT NOT NULL DEFAULT 0,"
				"created_at BIGINT NOT NULL,"
				"KEY idx_group_created(group_id, created_at),"
				"KEY idx_group_seq(group_id, group_seq)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_msg_ext ("
				"msg_id VARCHAR(64) PRIMARY KEY,"
				"file_name VARCHAR(255) DEFAULT '',"
				"mime VARCHAR(127) DEFAULT '',"
				"size INT NOT NULL DEFAULT 0"
				") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS chat_private_msg ("
				"msg_id VARCHAR(64) PRIMARY KEY,"
				"conv_uid_min INT NOT NULL,"
				"conv_uid_max INT NOT NULL,"
				"from_uid INT NOT NULL,"
				"to_uid INT NOT NULL,"
				"content TEXT NOT NULL,"
				"reply_to_server_msg_id BIGINT NOT NULL DEFAULT 0,"
				"forward_meta_json TEXT,"
				"edited_at_ms BIGINT NOT NULL DEFAULT 0,"
				"deleted_at_ms BIGINT NOT NULL DEFAULT 0,"
				"created_at BIGINT NOT NULL,"
				"KEY idx_conv_created(conv_uid_min, conv_uid_max, created_at),"
				"KEY idx_to_created(to_uid, created_at)"
				") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_friend_self_id_friend_id ON friend(self_id, friend_id)");
			ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_friend_apply_to_uid_id_from_uid ON friend_apply(to_uid, id, from_uid)");
			ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_friend_apply_tag_to_uid_from_uid_id ON friend_apply_tag(to_uid, from_uid, id)");
			ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_friend_tag_self_id_friend_id_id ON friend_tag(self_id, friend_id, id)");
			pool_->returnConnection(std::move(con));
		}
	}
	catch (sql::SQLException& e) {
		std::cerr << "init tag tables failed: " << e.what() << std::endl;
	}

	if (!EnsureGroupCodeSchemaAndBackfill()) {
		throw std::runtime_error("failed to ensure/backfill chat_group.group_code");
	}
	if (!EnsureDialogMetaSchema()) {
		throw std::runtime_error("failed to ensure chat_dialog schema");
	}
	if (!EnsurePrivateReadStateSchema()) {
		throw std::runtime_error("failed to ensure chat_private_read_state schema");
	}
	if (!EnsureGroupReadStateSchema()) {
		throw std::runtime_error("failed to ensure chat_group_read_state schema");
	}
	if (!EnsureGroupMessageOrderSchema()) {
		throw std::runtime_error("failed to ensure/backfill chat_group_msg sequence schema");
	}
	if (!EnsureGroupPermissionSchemaAndBackfill()) {
		throw std::runtime_error("failed to ensure/backfill chat_group_admin_permission schema");
	}
	WarmupRelationBootstrapQueries();
}

MysqlDao::~MysqlDao(){
	pool_->Close();
}

void MysqlDao::WarmupRelationBootstrapQueries() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		{
			std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
				"select apply.from_uid, apply.status, user.name, user.nick, user.sex, user.user_id "
				"from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
				"and apply.id > ? order by apply.id ASC LIMIT ? "));
			pstmt->setInt(1, -1);
			pstmt->setInt(2, 0);
			pstmt->setInt(3, 1);
			std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
			(void)res;
		}

		{
			std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
				"SELECT from_uid, tag FROM friend_apply_tag WHERE to_uid = ? AND from_uid IN (?) ORDER BY id ASC"));
			pstmt->setInt(1, -1);
			pstmt->setInt(2, -1);
			std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
			(void)res;
		}

		{
			std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
				"SELECT f.friend_id, f.back, u.name, u.nick, u.sex, u.user_id, u.`desc`, u.icon "
				"FROM friend AS f "
				"JOIN user AS u ON f.friend_id = u.uid "
				"WHERE f.self_id = ? LIMIT 1"));
			pstmt->setInt(1, -1);
			std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
			(void)res;
		}

		{
			std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
				"SELECT friend_id, tag FROM friend_tag WHERE self_id = ? AND friend_id IN (?) ORDER BY id ASC"));
			pstmt->setInt(1, -1);
			pstmt->setInt(2, -1);
			std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
			(void)res;
		}
	}
	catch (sql::SQLException& e) {
		std::cerr << "warmup relation bootstrap queries failed: " << e.what() << std::endl;
	}
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		std::unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));

		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);




		stmt->execute();


	   std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
	  std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
	  if (res->next()) {
	       int result = res->getInt("result");
	      std::cout << "Result: " << result << std::endl;
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

bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name);


		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}

		const std::string origin_pwd = res->getString("pwd");
		if (pwd != origin_pwd) {
			return false;
		}

		userInfo.name = name;
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.user_id = res->isNull("user_id") ? "" : res->getString("user_id");
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

bool MysqlDao::AddFriendApply(const int& from, const int& to)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
			"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
		pstmt->setInt(1, from); // from id
		pstmt->setInt(2, to);

		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}


	return true;
}

bool MysqlDao::AuthFriendApply(const int& from, const int& to) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
			"WHERE from_uid = ? AND to_uid = ?"));

		pstmt->setInt(1, to); // from id
		pstmt->setInt(2, from);

		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}


	return true;
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string back_name) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {


		con->_con->setAutoCommit(false);


		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
			));

		pstmt->setInt(1, from); // from id
		pstmt->setInt(2, to);
		pstmt->setString(3, back_name);

		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			con->_con->rollback();
			return false;
		}


		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
		));

		pstmt2->setInt(1, to); // from id
		pstmt2->setInt(2, from);
		pstmt2->setString(3, "");

		int rowAffected2 = pstmt2->executeUpdate();
		if (rowAffected2 < 0) {
			con->_con->rollback();
			return false;
		}


		con->_con->commit();
		std::cout << "addfriend insert friends success" << std::endl;

		return true;
	}
	catch (sql::SQLException& e) {

		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}


	return true;
}

bool MysqlDao::ReplaceApplyTags(const int& from, const int& to, const std::vector<std::string>& tags) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> delStmt(con->_con->prepareStatement(
			"DELETE FROM friend_apply_tag WHERE from_uid = ? AND to_uid = ?"));
		delStmt->setInt(1, from);
		delStmt->setInt(2, to);
		delStmt->executeUpdate();

		std::set<std::string> uniqTags;
		for (const auto& raw : tags) {
			auto begin = raw.find_first_not_of(" \t\r\n");
			if (begin == std::string::npos) {
				continue;
			}
			auto end = raw.find_last_not_of(" \t\r\n");
			std::string tag = raw.substr(begin, end - begin + 1);
			if (tag.size() > 64) {
				tag = tag.substr(0, 64);
			}
			if (!tag.empty()) {
				uniqTags.insert(tag);
			}
		}

		if (!uniqTags.empty()) {
			std::unique_ptr<sql::PreparedStatement> insStmt(con->_con->prepareStatement(
				"INSERT IGNORE INTO friend_apply_tag(from_uid, to_uid, tag) VALUES(?,?,?)"));
			for (const auto& tag : uniqTags) {
				insStmt->setInt(1, from);
				insStmt->setInt(2, to);
				insStmt->setString(3, tag);
				insStmt->executeUpdate();
			}
		}

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::ReplaceFriendTags(const int& self_id, const int& friend_id, const std::vector<std::string>& tags) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> delStmt(con->_con->prepareStatement(
			"DELETE FROM friend_tag WHERE self_id = ? AND friend_id = ?"));
		delStmt->setInt(1, self_id);
		delStmt->setInt(2, friend_id);
		delStmt->executeUpdate();

		std::set<std::string> uniqTags;
		for (const auto& raw : tags) {
			auto begin = raw.find_first_not_of(" \t\r\n");
			if (begin == std::string::npos) {
				continue;
			}
			auto end = raw.find_last_not_of(" \t\r\n");
			std::string tag = raw.substr(begin, end - begin + 1);
			if (tag.size() > 64) {
				tag = tag.substr(0, 64);
			}
			if (!tag.empty()) {
				uniqTags.insert(tag);
			}
		}

		if (!uniqTags.empty()) {
			std::unique_ptr<sql::PreparedStatement> insStmt(con->_con->prepareStatement(
				"INSERT IGNORE INTO friend_tag(self_id, friend_id, tag) VALUES(?,?,?)"));
			for (const auto& tag : uniqTags) {
				insStmt->setInt(1, self_id);
				insStmt->setInt(2, friend_id);
				insStmt->setString(3, tag);
				insStmt->executeUpdate();
			}
		}

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

std::vector<std::string> MysqlDao::GetApplyTags(const int& from, const int& to) {
	std::vector<std::string> tags;
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return tags;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT tag FROM friend_apply_tag WHERE from_uid = ? AND to_uid = ? ORDER BY id ASC"));
		pstmt->setInt(1, from);
		pstmt->setInt(2, to);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			tags.push_back(res->getString("tag"));
		}
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
	}
	return tags;
}

std::vector<std::string> MysqlDao::GetFriendTags(const int& self_id, const int& friend_id) {
	std::vector<std::string> tags;
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return tags;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT tag FROM friend_tag WHERE self_id = ? AND friend_id = ? ORDER BY id ASC"));
		pstmt->setInt(1, self_id);
		pstmt->setInt(2, friend_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			tags.push_back(res->getString("tag"));
		}
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
	}
	return tags;
}
std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid);


		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;

		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name= res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->icon = res->getString("icon");
			user_ptr->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
			user_ptr->uid = uid;
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name);


		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;

		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->icon = res->getString("icon");
			user_ptr->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
			user_ptr->uid = res->getInt("uid");
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

bool MysqlDao::GetUidByUserId(const std::string& user_id, int& uid) {
	uid = 0;
	if (!IsValidUserPublicId(user_id)) {
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
			con->_con->prepareStatement("SELECT uid FROM user WHERE user_id = ? LIMIT 1"));
		pstmt->setString(1, user_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		uid = res->getInt("uid");
		return uid > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


bool MysqlDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});


	try {
		std::vector<int> from_uids;
		std::unordered_map<int, std::shared_ptr<ApplyInfo>> apply_by_uid;

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
				"user.nick, user.sex, user.user_id from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
			"and apply.id > ? order by apply.id ASC LIMIT ? "));

		pstmt->setInt(1, touid);
		pstmt->setInt(2, begin);
		pstmt->setInt(3, limit);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {	
			auto name = res->getString("name");
			auto uid = res->getInt("from_uid");
			auto status = res->getInt("status");
			auto nick = res->getString("nick");
			auto sex = res->getInt("sex");
			auto user_id = res->isNull("user_id") ? "" : res->getString("user_id");
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status, user_id);
			applyList.push_back(apply_ptr);
			from_uids.push_back(uid);
			apply_by_uid.emplace(uid, apply_ptr);
		}

		if (!from_uids.empty()) {
			std::string placeholders;
			placeholders.reserve(from_uids.size() * 2);
			for (size_t i = 0; i < from_uids.size(); ++i) {
				if (i > 0) {
					placeholders += ",";
				}
				placeholders += "?";
			}

			std::unique_ptr<sql::PreparedStatement> tag_stmt(con->_con->prepareStatement(
				"SELECT from_uid, tag FROM friend_apply_tag WHERE to_uid = ? AND from_uid IN (" + placeholders + ") ORDER BY id ASC"));
			tag_stmt->setInt(1, touid);
			for (size_t i = 0; i < from_uids.size(); ++i) {
				tag_stmt->setInt(static_cast<int>(i + 2), from_uids[i]);
			}
			std::unique_ptr<sql::ResultSet> tag_res(tag_stmt->executeQuery());
			while (tag_res->next()) {
				const auto uid = tag_res->getInt("from_uid");
				const auto it = apply_by_uid.find(uid);
				if (it != apply_by_uid.end() && it->second) {
					it->second->_labels.push_back(tag_res->getString("tag"));
				}
			}
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info_list) {

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});


	try {
		std::vector<int> friend_ids;
		std::unordered_map<int, std::shared_ptr<UserInfo>> friend_by_id;

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT f.friend_id, f.back, u.name, u.nick, u.sex, u.user_id, u.`desc`, u.icon "
			"FROM friend AS f "
			"JOIN user AS u ON f.friend_id = u.uid "
			"WHERE f.self_id = ?"));

		pstmt->setInt(1, self_id);
	

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {		
			auto friend_id = res->getInt("friend_id");
			auto back = res->isNull("back") ? "" : res->getString("back");
			auto user_info = std::make_shared<UserInfo>();
			user_info->uid = friend_id;
			user_info->name = res->getString("name");
			user_info->nick = res->isNull("nick") ? "" : res->getString("nick");
			user_info->sex = res->getInt("sex");
			user_info->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
			user_info->desc = res->isNull("desc") ? "" : res->getString("desc");
			user_info->icon = res->isNull("icon") ? "" : res->getString("icon");
			user_info->back = back;
			user_info_list.push_back(user_info);
			friend_ids.push_back(friend_id);
			friend_by_id.emplace(friend_id, user_info);
		}

		if (!friend_ids.empty()) {
			std::string placeholders;
			placeholders.reserve(friend_ids.size() * 2);
			for (size_t i = 0; i < friend_ids.size(); ++i) {
				if (i > 0) {
					placeholders += ",";
				}
				placeholders += "?";
			}

			std::unique_ptr<sql::PreparedStatement> tag_stmt(con->_con->prepareStatement(
				"SELECT friend_id, tag FROM friend_tag WHERE self_id = ? AND friend_id IN (" + placeholders + ") ORDER BY id ASC"));
			tag_stmt->setInt(1, self_id);
			for (size_t i = 0; i < friend_ids.size(); ++i) {
				tag_stmt->setInt(static_cast<int>(i + 2), friend_ids[i]);
			}
			std::unique_ptr<sql::ResultSet> tag_res(tag_stmt->executeQuery());
			while (tag_res->next()) {
				const auto friend_id = tag_res->getInt("friend_id");
				const auto it = friend_by_id.find(friend_id);
				if (it != friend_by_id.end() && it->second) {
					it->second->labels.push_back(tag_res->getString("tag"));
				}
			}
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}

bool MysqlDao::SavePrivateMessage(const PrivateMessageInfo& msg) {
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
				"INSERT INTO chat_private_msg(msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
				"reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at) "
				"VALUES(?,?,?,?,?,?,?,?,?,?,?) "
				"ON DUPLICATE KEY UPDATE from_uid = VALUES(from_uid), to_uid = VALUES(to_uid), "
				"content = VALUES(content), reply_to_server_msg_id = VALUES(reply_to_server_msg_id), "
				"forward_meta_json = VALUES(forward_meta_json), edited_at_ms = VALUES(edited_at_ms), "
				"deleted_at_ms = VALUES(deleted_at_ms), created_at = VALUES(created_at)"));
		pstmt->setString(1, msg.msg_id);
		pstmt->setInt(2, msg.conv_uid_min);
		pstmt->setInt(3, msg.conv_uid_max);
		pstmt->setInt(4, msg.from_uid);
		pstmt->setInt(5, msg.to_uid);
		pstmt->setString(6, msg.content);
		pstmt->setInt64(7, msg.reply_to_server_msg_id);
		pstmt->setString(8, msg.forward_meta_json);
		pstmt->setInt64(9, msg.edited_at_ms);
		pstmt->setInt64(10, msg.deleted_at_ms);
		pstmt->setInt64(11, msg.created_at);
		const int row_affected = pstmt->executeUpdate();
		return row_affected >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetPrivateHistory(const int& uid, const int& peer_uid, const int64_t& before_ts, const std::string& before_msg_id, const int& limit,
	std::vector<std::shared_ptr<PrivateMessageInfo>>& messages, bool& has_more) {
	has_more = false;
	messages.clear();
	if (uid <= 0 || peer_uid <= 0 || limit <= 0) {
		return false;
	}

	const int conv_min = std::min(uid, peer_uid);
	const int conv_max = std::max(uid, peer_uid);

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt;
		if (before_ts > 0 && !before_msg_id.empty()) {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
				"reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
				"FROM chat_private_msg WHERE conv_uid_min = ? AND conv_uid_max = ? "
				"AND (created_at < ? OR (created_at = ? AND msg_id < ?)) "
				"ORDER BY created_at DESC, msg_id DESC LIMIT ?"));
			pstmt->setInt(1, conv_min);
			pstmt->setInt(2, conv_max);
			pstmt->setInt64(3, before_ts);
			pstmt->setInt64(4, before_ts);
			pstmt->setString(5, before_msg_id);
			pstmt->setInt(6, limit + 1);
		}
		else if (before_ts > 0) {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
				"reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
				"FROM chat_private_msg WHERE conv_uid_min = ? AND conv_uid_max = ? AND created_at < ? "
				"ORDER BY created_at DESC, msg_id DESC LIMIT ?"));
			pstmt->setInt(1, conv_min);
			pstmt->setInt(2, conv_max);
			pstmt->setInt64(3, before_ts);
			pstmt->setInt(4, limit + 1);
		}
		else {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
				"reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
				"FROM chat_private_msg WHERE conv_uid_min = ? AND conv_uid_max = ? "
				"ORDER BY created_at DESC, msg_id DESC LIMIT ?"));
			pstmt->setInt(1, conv_min);
			pstmt->setInt(2, conv_max);
			pstmt->setInt(3, limit + 1);
		}

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto one = std::make_shared<PrivateMessageInfo>();
			one->msg_id = res->getString("msg_id");
			one->conv_uid_min = res->getInt("conv_uid_min");
			one->conv_uid_max = res->getInt("conv_uid_max");
			one->from_uid = res->getInt("from_uid");
			one->to_uid = res->getInt("to_uid");
			one->content = res->getString("content");
			one->reply_to_server_msg_id = res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
			one->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
			one->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
			one->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
			one->created_at = res->getInt64("created_at");
			messages.push_back(one);
		}

		if (static_cast<int>(messages.size()) > limit) {
			has_more = true;
			messages.resize(limit);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message) {
	message = nullptr;
	if (msg_id.empty()) {
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
				"SELECT msg_id, conv_uid_min, conv_uid_max, from_uid, to_uid, content, "
				"reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at "
				"FROM chat_private_msg WHERE msg_id = ? LIMIT 1"));
		pstmt->setString(1, msg_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}

		auto one = std::make_shared<PrivateMessageInfo>();
		one->msg_id = res->getString("msg_id");
		one->conv_uid_min = res->getInt("conv_uid_min");
		one->conv_uid_max = res->getInt("conv_uid_max");
		one->from_uid = res->getInt("from_uid");
		one->to_uid = res->getInt("to_uid");
		one->content = res->getString("content");
		one->reply_to_server_msg_id = res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
		one->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
		one->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
		one->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
		one->created_at = res->getInt64("created_at");
		message = one;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePrivateMessageContent(const int& uid, const int& peer_uid, const std::string& msg_id,
	const std::string& content, int64_t edited_at_ms) {
	if (uid <= 0 || peer_uid <= 0 || msg_id.empty() || content.empty()) {
		return false;
	}
	if (edited_at_ms <= 0) {
		edited_at_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::shared_ptr<PrivateMessageInfo> message;
		if (!GetPrivateMessageByMsgId(msg_id, message) || !message) {
			return false;
		}
		const int conv_min = std::min(uid, peer_uid);
		const int conv_max = std::max(uid, peer_uid);
		if (message->conv_uid_min != conv_min || message->conv_uid_max != conv_max || message->from_uid != uid) {
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"UPDATE chat_private_msg SET content = ?, edited_at_ms = ?, deleted_at_ms = 0, "
				"created_at = created_at WHERE msg_id = ? AND conv_uid_min = ? AND conv_uid_max = ?"));
		pstmt->setString(1, content);
		pstmt->setInt64(2, edited_at_ms);
		pstmt->setString(3, msg_id);
		pstmt->setInt(4, conv_min);
		pstmt->setInt(5, conv_max);
		return pstmt->executeUpdate() > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::RevokePrivateMessage(const int& uid, const int& peer_uid, const std::string& msg_id, int64_t deleted_at_ms) {
	if (uid <= 0 || peer_uid <= 0 || msg_id.empty()) {
		return false;
	}
	if (deleted_at_ms <= 0) {
		deleted_at_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::shared_ptr<PrivateMessageInfo> message;
		if (!GetPrivateMessageByMsgId(msg_id, message) || !message) {
			return false;
		}
		const int conv_min = std::min(uid, peer_uid);
		const int conv_max = std::max(uid, peer_uid);
		if (message->conv_uid_min != conv_min || message->conv_uid_max != conv_max || message->from_uid != uid) {
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"UPDATE chat_private_msg SET content = '[消息已撤回]', deleted_at_ms = ?, edited_at_ms = 0, "
				"created_at = created_at WHERE msg_id = ? AND conv_uid_min = ? AND conv_uid_max = ?"));
		pstmt->setInt64(1, deleted_at_ms);
		pstmt->setString(2, msg_id);
		pstmt->setInt(3, conv_min);
		pstmt->setInt(4, conv_max);
		return pstmt->executeUpdate() > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpsertPrivateReadState(const int& uid, const int& peer_uid, const int64_t& read_ts) {
	if (uid <= 0 || peer_uid <= 0) {
		return false;
	}
	int64_t normalized_read_ts = read_ts;
	if (normalized_read_ts <= 0) {
		normalized_read_ts = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
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
				"INSERT INTO chat_private_read_state(uid, peer_uid, read_ts) VALUES(?, ?, ?) "
				"ON DUPLICATE KEY UPDATE read_ts = GREATEST(read_ts, VALUES(read_ts)), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, peer_uid);
		pstmt->setInt64(3, normalized_read_ts);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::IsFriend(const int& self_id, const int& friend_id) {
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
		pstmt->setInt(1, self_id);
		pstmt->setInt(2, friend_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		return res->next();
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::CreateGroup(const int& owner_uid, const std::string& name, const std::string& announcement,
	const int& member_limit, const std::vector<int>& initial_members, int64_t& out_group_id, std::string& out_group_code) {
	out_group_id = 0;
	out_group_code.clear();
	if (owner_uid <= 0 || name.empty()) {
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
		const int final_limit = std::max(2, std::min(member_limit, 200));
		std::unordered_set<int> member_set;
		for (int uid : initial_members) {
			if (uid > 0 && uid != owner_uid) {
				member_set.insert(uid);
			}
		}
		if (static_cast<int>(member_set.size()) + 1 > final_limit) {
			return false;
		}

		con->_con->setAutoCommit(false);

		std::string group_code;
		for (int i = 0; i < 20; ++i) {
			const std::string candidate = GenerateRandomGroupCode();
			std::unique_ptr<sql::PreparedStatement> check_group_code(
				con->_con->prepareStatement("SELECT 1 FROM chat_group WHERE group_code = ? LIMIT 1"));
			check_group_code->setString(1, candidate);
			std::unique_ptr<sql::ResultSet> check_group_code_res(check_group_code->executeQuery());
			if (!check_group_code_res->next()) {
				group_code = candidate;
				break;
			}
		}
		if (group_code.empty()) {
			con->_con->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> ins_group(
			con->_con->prepareStatement("INSERT INTO chat_group(group_code, name, owner_uid, announcement, member_limit, is_all_muted, status) "
				"VALUES(?,?,?,?,?,0,1)"));
		ins_group->setString(1, group_code);
		ins_group->setString(2, name);
		ins_group->setInt(3, owner_uid);
		ins_group->setString(4, announcement);
		ins_group->setInt(5, final_limit);
		if (ins_group->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> id_res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS group_id"));
		if (!id_res->next()) {
			con->_con->rollback();
			return false;
		}
		out_group_id = static_cast<int64_t>(std::stoll(id_res->getString("group_id")));

		std::unique_ptr<sql::PreparedStatement> ins_member(
			con->_con->prepareStatement("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
				"VALUES(?,?,?,?,?,1)"));

		ins_member->setInt64(1, out_group_id);
		ins_member->setInt(2, owner_uid);
		ins_member->setInt(3, 3);
		ins_member->setInt64(4, 0);
		ins_member->setInt(5, 0);
		if (ins_member->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		for (int uid : member_set) {
			ins_member->setInt64(1, out_group_id);
			ins_member->setInt(2, uid);
			ins_member->setInt(3, 1);
			ins_member->setInt64(4, 0);
			ins_member->setInt(5, 1);
			ins_member->executeUpdate();
		}

		con->_con->commit();
		out_group_code = group_code;
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetGroupIdByCode(const std::string& group_code, int64_t& out_group_id) {
	out_group_id = 0;
	if (!IsValidGroupCode(group_code)) {
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
			con->_con->prepareStatement("SELECT group_id FROM chat_group WHERE group_code = ? AND status = 1 LIMIT 1"));
		pstmt->setString(1, group_code);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		out_group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
		return out_group_id > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetUserGroupList(const int& uid, std::vector<std::shared_ptr<GroupInfo>>& group_list) {
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
				"SELECT g.group_id, g.group_code, g.name, g.icon, g.owner_uid, g.announcement, g.member_limit, g.is_all_muted, g.status, m.role, "
				"CASE "
				"WHEN m.role >= 3 THEN ? "
				"WHEN m.role = 2 THEN COALESCE(NULLIF(p.permission_bits, 0), ?) "
				"ELSE 0 END AS permission_bits, "
				"(SELECT COUNT(1) FROM chat_group_member gm WHERE gm.group_id = g.group_id AND gm.status = 1) AS member_count "
				"FROM chat_group_member m JOIN chat_group g ON m.group_id = g.group_id "
				"LEFT JOIN chat_group_admin_permission p ON p.group_id = m.group_id AND p.uid = m.uid "
				"WHERE m.uid = ? AND m.status = 1 AND g.status = 1 ORDER BY g.updated_at DESC"));
		pstmt->setInt64(1, kOwnerPermBits);
		pstmt->setInt64(2, kDefaultAdminPermBits);
		pstmt->setInt(3, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupInfo>();
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->group_code = res->isNull("group_code") ? "" : res->getString("group_code");
			info->name = res->getString("name");
			info->icon = res->isNull("icon") ? "" : res->getString("icon");
			info->owner_uid = res->getInt("owner_uid");
			info->announcement = res->getString("announcement");
			info->member_limit = res->getInt("member_limit");
			info->is_all_muted = res->getInt("is_all_muted");
			info->status = res->getInt("status");
			info->role = res->getInt("role");
			info->permission_bits = static_cast<int64_t>(std::stoll(res->getString("permission_bits")));
			info->member_count = res->getInt("member_count");
			group_list.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetGroupMemberList(const int64_t& group_id, std::vector<std::shared_ptr<GroupMemberInfo>>& member_list) {
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
				"SELECT m.group_id, m.uid, m.role, m.mute_until, m.status, u.name, u.nick, u.icon, u.user_id "
				"FROM chat_group_member m LEFT JOIN user u ON m.uid = u.uid "
				"WHERE m.group_id = ? AND m.status = 1 ORDER BY m.role DESC, m.id ASC"));
		pstmt->setInt64(1, group_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupMemberInfo>();
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->uid = res->getInt("uid");
			info->role = res->getInt("role");
			info->mute_until = static_cast<int64_t>(std::stoll(res->getString("mute_until")));
			info->status = res->getInt("status");
			info->name = res->isNull("name") ? "" : res->getString("name");
			info->nick = res->isNull("nick") ? "" : res->getString("nick");
			info->icon = res->isNull("icon") ? "" : res->getString("icon");
			info->user_id = res->isNull("user_id") ? "" : res->getString("user_id");
			member_list.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::InviteGroupMember(const int64_t& group_id, const int& inviter_uid, const int& target_uid, const std::string& reason) {
	if (!HasGroupPermission(group_id, inviter_uid, kPermInviteUsers)) {
		return false;
	}
	if (!IsFriend(inviter_uid, target_uid)) {
		return false;
	}
	if (IsUserInGroup(group_id, target_uid)) {
		return true;
	}

	std::shared_ptr<GroupInfo> group_info;
	if (!GetGroupById(group_id, group_info) || !group_info) {
		return false;
	}
	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	if (!GetGroupMemberList(group_id, members)) {
		return false;
	}
	if (static_cast<int>(members.size()) >= group_info->member_limit) {
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
			con->_con->prepareStatement("INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, reason, reviewer_uid) "
				"VALUES(?,?,?,1,0,?,0)"));
		pstmt->setInt64(1, group_id);
		pstmt->setInt(2, target_uid);
		pstmt->setInt(3, inviter_uid);
		pstmt->setString(4, reason);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::ApplyJoinGroup(const int64_t& group_id, const int& applicant_uid, const std::string& reason) {
	if (IsUserInGroup(group_id, applicant_uid)) {
		return true;
	}
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> upd(
			con->_con->prepareStatement("UPDATE chat_group_apply SET reason = ?, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND applicant_uid = ? AND type = 2 AND status = 0"));
		upd->setString(1, reason);
		upd->setInt64(2, group_id);
		upd->setInt(3, applicant_uid);
		if (upd->executeUpdate() > 0) {
			return true;
		}
		std::unique_ptr<sql::PreparedStatement> ins(
			con->_con->prepareStatement("INSERT INTO chat_group_apply(group_id, applicant_uid, inviter_uid, type, status, reason, reviewer_uid) "
				"VALUES(?,?,0,2,0,?,0)"));
		ins->setInt64(1, group_id);
		ins->setInt(2, applicant_uid);
		ins->setString(3, reason);
		return ins->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::ReviewGroupApply(const int64_t& apply_id, const int& reviewer_uid, const bool& agree, std::shared_ptr<GroupApplyInfo>& apply_info) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> query(
			con->_con->prepareStatement("SELECT apply_id, group_id, applicant_uid, inviter_uid, type, status, reason "
				"FROM chat_group_apply WHERE apply_id = ? LIMIT 1"));
		query->setInt64(1, apply_id);
		std::unique_ptr<sql::ResultSet> res(query->executeQuery());
		if (!res->next()) {
			con->_con->rollback();
			return false;
		}

		auto found = std::make_shared<GroupApplyInfo>();
		found->apply_id = static_cast<int64_t>(std::stoll(res->getString("apply_id")));
		found->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
		found->applicant_uid = res->getInt("applicant_uid");
		found->inviter_uid = res->getInt("inviter_uid");
		found->type = (res->getInt("type") == 1) ? "invite" : "apply";
		found->status = res->getInt("status");
		found->reason = res->getString("reason");
		found->reviewer_uid = reviewer_uid;

		if (!HasGroupPermission(found->group_id, reviewer_uid, kPermInviteUsers)) {
			con->_con->rollback();
			return false;
		}

		if (found->status != 0) {
			apply_info = found;
			con->_con->commit();
			return true;
		}

		std::unique_ptr<sql::PreparedStatement> upd(
			con->_con->prepareStatement("UPDATE chat_group_apply SET status = ?, reviewer_uid = ?, updated_at = CURRENT_TIMESTAMP "
				"WHERE apply_id = ?"));
		upd->setInt(1, agree ? 1 : 2);
		upd->setInt(2, reviewer_uid);
		upd->setInt64(3, apply_id);
		if (upd->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		if (agree && !IsUserInGroup(found->group_id, found->applicant_uid)) {
			std::unique_ptr<sql::PreparedStatement> ins_member(
				con->_con->prepareStatement("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
					"VALUES(?,?,1,0,?,1) ON DUPLICATE KEY UPDATE status = 1, role = 1, mute_until = 0, updated_at = CURRENT_TIMESTAMP"));
			ins_member->setInt64(1, found->group_id);
			ins_member->setInt(2, found->applicant_uid);
			ins_member->setInt(3, found->type == "invite" ? 1 : 2);
			ins_member->executeUpdate();
			std::unique_ptr<sql::PreparedStatement> del_perm(
				con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
			del_perm->setInt64(1, found->group_id);
			del_perm->setInt(2, found->applicant_uid);
			del_perm->executeUpdate();
		}

		con->_con->commit();
		found->status = agree ? 1 : 2;
		apply_info = found;
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::SaveGroupMessage(const GroupMessageInfo& msg, int64_t* out_server_msg_id, int64_t* out_group_seq) {
	if (msg.msg_id.empty() || msg.group_id <= 0 || msg.from_uid <= 0) {
		return false;
	}
	if (out_server_msg_id) {
		*out_server_msg_id = 0;
	}
	if (out_group_seq) {
		*out_group_seq = 0;
	}
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		con->_con->setAutoCommit(false);
		{
			std::unique_ptr<sql::PreparedStatement> lock_group_stmt(
				con->_con->prepareStatement("SELECT group_id FROM chat_group WHERE group_id = ? AND status = 1 LIMIT 1 FOR UPDATE"));
			lock_group_stmt->setInt64(1, msg.group_id);
			std::unique_ptr<sql::ResultSet> lock_group_res(lock_group_stmt->executeQuery());
			if (!lock_group_res->next()) {
				con->_con->rollback();
				return false;
			}
		}
		int64_t next_group_seq = 0;
		{
			std::unique_ptr<sql::PreparedStatement> seq_stmt(
				con->_con->prepareStatement("SELECT COALESCE(MAX(group_seq), 0) + 1 AS next_seq "
					"FROM chat_group_msg WHERE group_id = ?"));
			seq_stmt->setInt64(1, msg.group_id);
			std::unique_ptr<sql::ResultSet> seq_res(seq_stmt->executeQuery());
			if (!seq_res->next()) {
				con->_con->rollback();
				return false;
			}
			next_group_seq = seq_res->getInt64("next_seq");
			if (next_group_seq <= 0) {
				next_group_seq = 1;
			}
		}

		std::unique_ptr<sql::PreparedStatement> ins_msg(
			con->_con->prepareStatement("INSERT INTO chat_group_msg(msg_id, group_id, group_seq, from_uid, msg_type, content, "
				"mentions_json, reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at) "
				"VALUES(?,?,?,?,?,?,?,?,?,?,?,?)"));
		ins_msg->setString(1, msg.msg_id);
		ins_msg->setInt64(2, msg.group_id);
		ins_msg->setInt64(3, next_group_seq);
		ins_msg->setInt(4, msg.from_uid);
		ins_msg->setString(5, msg.msg_type);
		ins_msg->setString(6, msg.content);
		ins_msg->setString(7, msg.mentions_json);
		ins_msg->setInt64(8, msg.reply_to_server_msg_id);
		ins_msg->setString(9, msg.forward_meta_json);
		ins_msg->setInt64(10, msg.edited_at_ms);
		ins_msg->setInt64(11, msg.deleted_at_ms);
		ins_msg->setInt64(12, msg.created_at);
		if (ins_msg->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		int64_t server_msg_id = 0;
		{
			std::unique_ptr<sql::Statement> id_stmt(con->_con->createStatement());
			std::unique_ptr<sql::ResultSet> id_res(id_stmt->executeQuery("SELECT LAST_INSERT_ID() AS server_msg_id"));
			if (id_res->next()) {
				server_msg_id = id_res->getInt64("server_msg_id");
			}
		}
		if (server_msg_id <= 0) {
			std::unique_ptr<sql::PreparedStatement> fallback_stmt(
				con->_con->prepareStatement("SELECT server_msg_id FROM chat_group_msg WHERE msg_id = ? LIMIT 1"));
			fallback_stmt->setString(1, msg.msg_id);
			std::unique_ptr<sql::ResultSet> fallback_res(fallback_stmt->executeQuery());
			if (fallback_res->next()) {
				server_msg_id = fallback_res->getInt64("server_msg_id");
			}
		}

		if (!msg.file_name.empty() || !msg.mime.empty() || msg.size > 0) {
			std::unique_ptr<sql::PreparedStatement> up_ext(
				con->_con->prepareStatement("REPLACE INTO chat_group_msg_ext(msg_id, file_name, mime, size) VALUES(?,?,?,?)"));
			up_ext->setString(1, msg.msg_id);
			up_ext->setString(2, msg.file_name);
			up_ext->setString(3, msg.mime);
			up_ext->setInt(4, msg.size);
			up_ext->executeUpdate();
		}

		con->_con->commit();
		if (out_server_msg_id) {
			*out_server_msg_id = server_msg_id;
		}
		if (out_group_seq) {
			*out_group_seq = next_group_seq;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdateGroupMessageContent(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
	const std::string& content, int64_t edited_at_ms) {
	if (group_id <= 0 || operator_uid <= 0 || msg_id.empty() || content.empty()) {
		return false;
	}
	if (edited_at_ms <= 0) {
		edited_at_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> qmsg(
			con->_con->prepareStatement("SELECT from_uid FROM chat_group_msg WHERE group_id = ? AND msg_id = ? LIMIT 1"));
		qmsg->setInt64(1, group_id);
		qmsg->setString(2, msg_id);
		std::unique_ptr<sql::ResultSet> res(qmsg->executeQuery());
		if (!res->next()) {
			return false;
		}
		const int from_uid = res->getInt("from_uid");

		if (from_uid != operator_uid) {
			if (!HasGroupPermission(group_id, operator_uid, kPermDeleteMessages)) {
				return false;
			}
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> up(
			con->_con->prepareStatement("UPDATE chat_group_msg SET content = ?, msg_type = 'text', mentions_json = '[]', "
				"edited_at_ms = ?, deleted_at_ms = 0 WHERE group_id = ? AND msg_id = ?"));
		up->setString(1, content);
		up->setInt64(2, edited_at_ms);
		up->setInt64(3, group_id);
		up->setString(4, msg_id);
		if (up->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> del_ext(
			con->_con->prepareStatement("DELETE FROM chat_group_msg_ext WHERE msg_id = ?"));
		del_ext->setString(1, msg_id);
		del_ext->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::RevokeGroupMessage(const int64_t& group_id, const int& operator_uid, const std::string& msg_id, int64_t deleted_at_ms) {
	if (group_id <= 0 || operator_uid <= 0 || msg_id.empty()) {
		return false;
	}
	if (deleted_at_ms <= 0) {
		deleted_at_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> qmsg(
			con->_con->prepareStatement("SELECT from_uid FROM chat_group_msg WHERE group_id = ? AND msg_id = ? LIMIT 1"));
		qmsg->setInt64(1, group_id);
		qmsg->setString(2, msg_id);
		std::unique_ptr<sql::ResultSet> res(qmsg->executeQuery());
		if (!res->next()) {
			return false;
		}
		const int from_uid = res->getInt("from_uid");

		if (from_uid != operator_uid) {
			if (!HasGroupPermission(group_id, operator_uid, kPermDeleteMessages)) {
				return false;
			}
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> up(
			con->_con->prepareStatement("UPDATE chat_group_msg SET content = '[消息已撤回]', msg_type = 'revoke', mentions_json = '[]', "
				"deleted_at_ms = ?, edited_at_ms = 0 WHERE group_id = ? AND msg_id = ?"));
		up->setInt64(1, deleted_at_ms);
		up->setInt64(2, group_id);
		up->setString(3, msg_id);
		if (up->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> del_ext(
			con->_con->prepareStatement("DELETE FROM chat_group_msg_ext WHERE msg_id = ?"));
		del_ext->setString(1, msg_id);
		del_ext->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int64_t& before_seq, const int& limit,
	std::vector<std::shared_ptr<GroupMessageInfo>>& messages, bool& has_more) {
	has_more = false;
	messages.clear();
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		const int final_limit = std::max(1, std::min(limit, 50));
		std::unique_ptr<sql::PreparedStatement> pstmt;
		if (before_seq > 0) {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"LEFT JOIN user u ON m.from_uid = u.uid "
				"WHERE m.group_id = ? AND m.group_seq < ? ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT ?"));
			pstmt->setInt64(1, group_id);
			pstmt->setInt64(2, before_seq);
			pstmt->setInt(3, final_limit + 1);
		}
		else if (before_ts > 0) {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"LEFT JOIN user u ON m.from_uid = u.uid "
				"WHERE m.group_id = ? AND m.created_at < ? ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT ?"));
			pstmt->setInt64(1, group_id);
			pstmt->setInt64(2, before_ts);
			pstmt->setInt(3, final_limit + 1);
		}
		else {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"LEFT JOIN user u ON m.from_uid = u.uid "
				"WHERE m.group_id = ? ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT ?"));
			pstmt->setInt64(1, group_id);
			pstmt->setInt(2, final_limit + 1);
		}
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupMessageInfo>();
			info->msg_id = res->getString("msg_id");
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->from_uid = res->getInt("from_uid");
			info->msg_type = res->getString("msg_type");
			info->content = res->getString("content");
			info->mentions_json = res->getString("mentions_json");
			info->reply_to_server_msg_id = res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
			info->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
			info->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
			info->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
			info->created_at = static_cast<int64_t>(std::stoll(res->getString("created_at")));
			info->server_msg_id = static_cast<int64_t>(std::stoll(res->getString("server_msg_id")));
			info->group_seq = static_cast<int64_t>(std::stoll(res->getString("group_seq")));
			info->file_name = res->isNull("file_name") ? "" : res->getString("file_name");
			info->mime = res->isNull("mime") ? "" : res->getString("mime");
			info->size = res->isNull("size") ? 0 : res->getInt("size");
			info->from_name = res->isNull("from_name") ? "" : res->getString("from_name");
			info->from_nick = res->isNull("from_nick") ? "" : res->getString("from_nick");
			info->from_icon = res->isNull("from_icon") ? "" : res->getString("from_icon");
			messages.push_back(info);
		}
		if (static_cast<int>(messages.size()) > final_limit) {
			has_more = true;
			messages.resize(final_limit);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdateGroupAnnouncement(const int64_t& group_id, const int& operator_uid, const std::string& announcement) {
	if (!HasGroupPermission(group_id, operator_uid, kPermChangeGroupInfo)) {
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
			con->_con->prepareStatement("UPDATE chat_group SET announcement = ?, updated_at = CURRENT_TIMESTAMP WHERE group_id = ? AND status = 1"));
		pstmt->setString(1, announcement);
		pstmt->setInt64(2, group_id);
		return pstmt->executeUpdate() > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdateGroupIcon(const int64_t& group_id, const int& operator_uid, const std::string& icon) {
	if (!HasGroupPermission(group_id, operator_uid, kPermChangeGroupInfo)) {
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
			con->_con->prepareStatement("UPDATE chat_group SET icon = ?, updated_at = CURRENT_TIMESTAMP WHERE group_id = ? AND status = 1"));
		pstmt->setString(1, icon);
		pstmt->setInt64(2, group_id);
		return pstmt->executeUpdate() > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::SetGroupAdmin(const int64_t& group_id, const int& operator_uid, const int& target_uid, const bool& is_admin, const int64_t& permission_bits) {
	if (!HasGroupPermission(group_id, operator_uid, kPermManageAdmins)) {
		return false;
	}
	int operator_role = 0;
	if (!GetUserRoleInGroup(group_id, operator_uid, operator_role) || operator_role < 2) {
		return false;
	}
	int target_role = 0;
	if (!GetUserRoleInGroup(group_id, target_uid, target_role) || target_role == 3) {
		return false;
	}
	if (operator_role < 3 && target_role >= operator_role) {
		return false;
	}
	int64_t normalized_perm_bits = kDefaultAdminPermBits;
	if (permission_bits > 0) {
		normalized_perm_bits = permission_bits & kOwnerPermBits;
		if (normalized_perm_bits <= 0) {
			normalized_perm_bits = kDefaultAdminPermBits;
		}
	}
	if (!is_admin) {
		normalized_perm_bits = 0;
	}
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("UPDATE chat_group_member SET role = ?, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND uid = ? AND status = 1"));
		pstmt->setInt(1, is_admin ? 2 : 1);
		pstmt->setInt64(2, group_id);
		pstmt->setInt(3, target_uid);
		if (pstmt->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		if (is_admin) {
			std::unique_ptr<sql::PreparedStatement> up_perm(
				con->_con->prepareStatement("INSERT INTO chat_group_admin_permission(group_id, uid, permission_bits) "
					"VALUES(?,?,?) ON DUPLICATE KEY UPDATE permission_bits = VALUES(permission_bits), updated_at = CURRENT_TIMESTAMP"));
			up_perm->setInt64(1, group_id);
			up_perm->setInt(2, target_uid);
			up_perm->setInt64(3, normalized_perm_bits);
			up_perm->executeUpdate();
		}
		else {
			std::unique_ptr<sql::PreparedStatement> del_perm(
				con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
			del_perm->setInt64(1, group_id);
			del_perm->setInt(2, target_uid);
			del_perm->executeUpdate();
		}
		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		try {
			con->_con->rollback();
		}
		catch (...) {
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::MuteGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid, const int64_t& mute_until) {
	int op_role = 0;
	if (!GetUserRoleInGroup(group_id, operator_uid, op_role) || op_role < 2
		|| !HasGroupPermission(group_id, operator_uid, kPermBanUsers)) {
		return false;
	}
	int target_role = 0;
	if (!GetUserRoleInGroup(group_id, target_uid, target_role) || target_role >= op_role) {
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
			con->_con->prepareStatement("UPDATE chat_group_member SET mute_until = ?, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND uid = ? AND status = 1"));
		pstmt->setInt64(1, std::max<int64_t>(0, mute_until));
		pstmt->setInt64(2, group_id);
		pstmt->setInt(3, target_uid);
		return pstmt->executeUpdate() > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::KickGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid) {
	int op_role = 0;
	if (!GetUserRoleInGroup(group_id, operator_uid, op_role) || op_role < 2
		|| !HasGroupPermission(group_id, operator_uid, kPermBanUsers)) {
		return false;
	}
	int target_role = 0;
	if (!GetUserRoleInGroup(group_id, target_uid, target_role) || target_role >= op_role) {
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
			con->_con->prepareStatement("UPDATE chat_group_member SET status = 3, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND uid = ? AND status = 1"));
		pstmt->setInt64(1, group_id);
		pstmt->setInt(2, target_uid);
		if (pstmt->executeUpdate() <= 0) {
			return false;
		}
		std::unique_ptr<sql::PreparedStatement> del_perm(
			con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
		del_perm->setInt64(1, group_id);
		del_perm->setInt(2, target_uid);
		del_perm->executeUpdate();
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::QuitGroup(const int64_t& group_id, const int& uid) {
	int role = 0;
	if (!GetUserRoleInGroup(group_id, uid, role)) {
		return false;
	}
	if (role == 3) {
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
			con->_con->prepareStatement("UPDATE chat_group_member SET status = 2, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND uid = ? AND status = 1"));
		pstmt->setInt64(1, group_id);
		pstmt->setInt(2, uid);
		if (pstmt->executeUpdate() <= 0) {
			return false;
		}
		std::unique_ptr<sql::PreparedStatement> del_perm(
			con->_con->prepareStatement("DELETE FROM chat_group_admin_permission WHERE group_id = ? AND uid = ?"));
		del_perm->setInt64(1, group_id);
		del_perm->setInt(2, uid);
		del_perm->executeUpdate();
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetGroupById(const int64_t& group_id, std::shared_ptr<GroupInfo>& group_info) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("SELECT group_id, group_code, name, icon, owner_uid, announcement, member_limit, is_all_muted, status "
				"FROM chat_group WHERE group_id = ? LIMIT 1"));
		pstmt->setInt64(1, group_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		auto info = std::make_shared<GroupInfo>();
		info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
		info->group_code = res->isNull("group_code") ? "" : res->getString("group_code");
		info->name = res->getString("name");
		info->icon = res->isNull("icon") ? "" : res->getString("icon");
		info->owner_uid = res->getInt("owner_uid");
		info->announcement = res->getString("announcement");
		info->member_limit = res->getInt("member_limit");
		info->is_all_muted = res->getInt("is_all_muted");
		info->status = res->getInt("status");
		group_info = info;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetUserRoleInGroup(const int64_t& group_id, const int& uid, int& role) {
	role = 0;
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("SELECT role FROM chat_group_member WHERE group_id = ? AND uid = ? AND status = 1 LIMIT 1"));
		pstmt->setInt64(1, group_id);
		pstmt->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		role = res->getInt("role");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::IsUserInGroup(const int64_t& group_id, const int& uid) {
	int role = 0;
	return GetUserRoleInGroup(group_id, uid, role);
}

bool MysqlDao::GetGroupPermissionBits(const int64_t& group_id, const int& uid, int64_t& out_bits) {
	out_bits = 0;
	int role = 0;
	if (!GetUserRoleInGroup(group_id, uid, role)) {
		return false;
	}
	if (role >= 3) {
		out_bits = kOwnerPermBits;
		return true;
	}
	if (role < 2) {
		out_bits = 0;
		return true;
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
			con->_con->prepareStatement("SELECT permission_bits FROM chat_group_admin_permission "
				"WHERE group_id = ? AND uid = ? LIMIT 1"));
		pstmt->setInt64(1, group_id);
		pstmt->setInt(2, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			out_bits = kDefaultAdminPermBits;
			return true;
		}
		out_bits = static_cast<int64_t>(std::stoll(res->getString("permission_bits")));
		if (out_bits <= 0) {
			out_bits = kDefaultAdminPermBits;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::HasGroupPermission(const int64_t& group_id, const int& uid, int64_t required_bits) {
	int64_t bits = 0;
	if (!GetGroupPermissionBits(group_id, uid, bits)) {
		return false;
	}
	return (bits & required_bits) == required_bits;
}

bool MysqlDao::GetDialogMetaByOwner(const int& owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas) {
	metas.clear();
	if (owner_uid <= 0) {
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
				"SELECT dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state "
				"FROM chat_dialog WHERE owner_uid = ?"));
		pstmt->setInt(1, owner_uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<DialogMetaInfo>();
			info->owner_uid = owner_uid;
			info->dialog_type = res->getString("dialog_type");
			info->peer_uid = res->getInt("peer_uid");
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->pinned_rank = res->getInt("pinned_rank");
			info->draft_text = res->isNull("draft_text") ? "" : res->getString("draft_text");
			info->mute_state = res->getInt("mute_state");
			metas.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetPrivateDialogRuntime(const int& owner_uid, const int& peer_uid, DialogRuntimeInfo& runtime) {
	runtime = DialogRuntimeInfo();
	if (owner_uid <= 0 || peer_uid <= 0) {
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
				"SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
				"WHERE owner_uid = ? AND dialog_type = 'private' AND peer_uid = ? AND group_id = 0 LIMIT 1"));
		pstmt->setInt(1, owner_uid);
		pstmt->setInt(2, peer_uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (res->next()) {
			runtime.last_msg_preview = res->isNull("last_msg_preview") ? "" : res->getString("last_msg_preview");
			runtime.last_msg_ts = res->isNull("last_msg_ts") ? 0 : res->getInt64("last_msg_ts");
			runtime.unread_count = std::max(0, res->getInt("unread_count"));
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetGroupDialogRuntime(const int& owner_uid, const int64_t& group_id, DialogRuntimeInfo& runtime) {
	runtime = DialogRuntimeInfo();
	if (owner_uid <= 0 || group_id <= 0) {
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
				"SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
				"WHERE owner_uid = ? AND dialog_type = 'group' AND group_id = ? LIMIT 1"));
		pstmt->setInt(1, owner_uid);
		pstmt->setInt64(2, group_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (res->next()) {
			runtime.last_msg_preview = res->isNull("last_msg_preview") ? "" : res->getString("last_msg_preview");
			runtime.last_msg_ts = res->isNull("last_msg_ts") ? 0 : res->getInt64("last_msg_ts");
			runtime.unread_count = std::max(0, res->getInt("unread_count"));
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::RefreshDialogsForOwner(const int& owner_uid) {
	if (owner_uid <= 0) {
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
		std::vector<int> peers;
		{
			std::unique_ptr<sql::PreparedStatement> list_private(
				con->_con->prepareStatement("SELECT friend_id FROM friend WHERE self_id = ?"));
			list_private->setInt(1, owner_uid);
			std::unique_ptr<sql::ResultSet> private_res(list_private->executeQuery());
			while (private_res->next()) {
				const int peer_uid = private_res->getInt("friend_id");
				if (peer_uid > 0) {
					peers.push_back(peer_uid);
				}
			}
		}

		for (int peer_uid : peers) {
			std::unique_ptr<sql::PreparedStatement> ensure_row(
				con->_con->prepareStatement(
					"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
					"VALUES(?, 'private', ?, 0, 0, '', 0, '', 0, 0) "
					"ON DUPLICATE KEY UPDATE owner_uid = owner_uid"));
			ensure_row->setInt(1, owner_uid);
			ensure_row->setInt(2, peer_uid);
			ensure_row->executeUpdate();

			const int conv_min = std::min(owner_uid, peer_uid);
			const int conv_max = std::max(owner_uid, peer_uid);
			std::string preview;
			int64_t last_msg_ts = 0;
			int unread_count = 0;

			{
				std::unique_ptr<sql::PreparedStatement> last_stmt(
					con->_con->prepareStatement(
						"SELECT content, created_at FROM chat_private_msg "
						"WHERE conv_uid_min = ? AND conv_uid_max = ? "
						"ORDER BY created_at DESC, msg_id DESC LIMIT 1"));
				last_stmt->setInt(1, conv_min);
				last_stmt->setInt(2, conv_max);
				std::unique_ptr<sql::ResultSet> last_res(last_stmt->executeQuery());
				if (last_res->next()) {
					const std::string content = last_res->isNull("content") ? "" : last_res->getString("content");
					preview = BuildPreviewText("", content);
					last_msg_ts = last_res->isNull("created_at") ? 0 : last_res->getInt64("created_at");
				}
			}

			{
				std::unique_ptr<sql::PreparedStatement> unread_stmt(
					con->_con->prepareStatement(
						"SELECT COUNT(1) AS unread_count FROM chat_private_msg "
						"WHERE conv_uid_min = ? AND conv_uid_max = ? AND from_uid = ? "
						"AND created_at > COALESCE((SELECT read_ts FROM chat_private_read_state WHERE uid = ? AND peer_uid = ?), 0)"));
				unread_stmt->setInt(1, conv_min);
				unread_stmt->setInt(2, conv_max);
				unread_stmt->setInt(3, peer_uid);
				unread_stmt->setInt(4, owner_uid);
				unread_stmt->setInt(5, peer_uid);
				std::unique_ptr<sql::ResultSet> unread_res(unread_stmt->executeQuery());
				if (unread_res->next()) {
					unread_count = std::max(0, unread_res->getInt("unread_count"));
				}
			}

			std::unique_ptr<sql::PreparedStatement> update_stmt(
				con->_con->prepareStatement(
					"UPDATE chat_dialog SET last_msg_preview = ?, last_msg_ts = ?, unread_count = ?, updated_at = CURRENT_TIMESTAMP "
					"WHERE owner_uid = ? AND dialog_type = 'private' AND peer_uid = ? AND group_id = 0"));
			update_stmt->setString(1, preview);
			update_stmt->setInt64(2, last_msg_ts);
			update_stmt->setInt(3, unread_count);
			update_stmt->setInt(4, owner_uid);
			update_stmt->setInt(5, peer_uid);
			update_stmt->executeUpdate();
		}

		std::vector<int64_t> groups;
		{
			std::unique_ptr<sql::PreparedStatement> list_groups(
				con->_con->prepareStatement(
					"SELECT m.group_id FROM chat_group_member m "
					"JOIN chat_group g ON g.group_id = m.group_id "
					"WHERE m.uid = ? AND m.status = 1 AND g.status = 1"));
			list_groups->setInt(1, owner_uid);
			std::unique_ptr<sql::ResultSet> group_res(list_groups->executeQuery());
			while (group_res->next()) {
				const int64_t group_id = group_res->getInt64("group_id");
				if (group_id > 0) {
					groups.push_back(group_id);
				}
			}
		}

		for (int64_t group_id : groups) {
			std::unique_ptr<sql::PreparedStatement> ensure_row(
				con->_con->prepareStatement(
					"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
					"VALUES(?, 'group', 0, ?, 0, '', 0, '', 0, 0) "
					"ON DUPLICATE KEY UPDATE owner_uid = owner_uid"));
			ensure_row->setInt(1, owner_uid);
			ensure_row->setInt64(2, group_id);
			ensure_row->executeUpdate();

			std::string preview;
			int64_t last_msg_ts = 0;
			int unread_count = 0;

			{
				std::unique_ptr<sql::PreparedStatement> last_stmt(
					con->_con->prepareStatement(
						"SELECT msg_type, content, created_at FROM chat_group_msg "
						"WHERE group_id = ? ORDER BY group_seq DESC, server_msg_id DESC LIMIT 1"));
				last_stmt->setInt64(1, group_id);
				std::unique_ptr<sql::ResultSet> last_res(last_stmt->executeQuery());
				if (last_res->next()) {
					const std::string msg_type = last_res->isNull("msg_type") ? "" : last_res->getString("msg_type");
					const std::string content = last_res->isNull("content") ? "" : last_res->getString("content");
					preview = BuildPreviewText(msg_type, content);
					last_msg_ts = last_res->isNull("created_at") ? 0 : last_res->getInt64("created_at");
				}
			}

			{
				std::unique_ptr<sql::PreparedStatement> unread_stmt(
					con->_con->prepareStatement(
						"SELECT COUNT(1) AS unread_count FROM chat_group_msg "
						"WHERE group_id = ? AND created_at > COALESCE((SELECT read_ts FROM chat_group_read_state WHERE uid = ? AND group_id = ?), 0) "
						"AND from_uid <> ?"));
				unread_stmt->setInt64(1, group_id);
				unread_stmt->setInt(2, owner_uid);
				unread_stmt->setInt64(3, group_id);
				unread_stmt->setInt(4, owner_uid);
				std::unique_ptr<sql::ResultSet> unread_res(unread_stmt->executeQuery());
				if (unread_res->next()) {
					unread_count = std::max(0, unread_res->getInt("unread_count"));
				}
			}

			std::unique_ptr<sql::PreparedStatement> update_stmt(
				con->_con->prepareStatement(
					"UPDATE chat_dialog SET last_msg_preview = ?, last_msg_ts = ?, unread_count = ?, updated_at = CURRENT_TIMESTAMP "
					"WHERE owner_uid = ? AND dialog_type = 'group' AND group_id = ?"));
			update_stmt->setString(1, preview);
			update_stmt->setInt64(2, last_msg_ts);
			update_stmt->setInt(3, unread_count);
			update_stmt->setInt(4, owner_uid);
			update_stmt->setInt64(5, group_id);
			update_stmt->executeUpdate();
		}

		{
			std::unique_ptr<sql::PreparedStatement> clean_private(
				con->_con->prepareStatement(
					"DELETE FROM chat_dialog "
					"WHERE owner_uid = ? AND dialog_type = 'private' AND peer_uid > 0 "
					"AND NOT EXISTS (SELECT 1 FROM friend f WHERE f.self_id = ? AND f.friend_id = chat_dialog.peer_uid)"));
			clean_private->setInt(1, owner_uid);
			clean_private->setInt(2, owner_uid);
			clean_private->executeUpdate();
		}
		{
			std::unique_ptr<sql::PreparedStatement> clean_group(
				con->_con->prepareStatement(
					"DELETE FROM chat_dialog "
					"WHERE owner_uid = ? AND dialog_type = 'group' AND group_id > 0 "
					"AND NOT EXISTS ("
					"SELECT 1 FROM chat_group_member m JOIN chat_group g ON g.group_id = m.group_id "
					"WHERE m.uid = ? AND m.group_id = chat_dialog.group_id AND m.status = 1 AND g.status = 1)"));
			clean_group->setInt(1, owner_uid);
			clean_group->setInt(2, owner_uid);
			clean_group->executeUpdate();
		}

		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpsertGroupReadState(const int& uid, const int64_t& group_id, const int64_t& read_ts) {
	if (uid <= 0 || group_id <= 0) {
		return false;
	}
	int64_t normalized_read_ts = read_ts;
	if (normalized_read_ts <= 0) {
		normalized_read_ts = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
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
				"INSERT INTO chat_group_read_state(uid, group_id, read_ts) VALUES(?, ?, ?) "
				"ON DUPLICATE KEY UPDATE read_ts = GREATEST(read_ts, VALUES(read_ts)), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, uid);
		pstmt->setInt64(2, group_id);
		pstmt->setInt64(3, normalized_read_ts);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetGroupMessageByMsgId(const int64_t& group_id, const std::string& msg_id, std::shared_ptr<GroupMessageInfo>& message) {
	message = nullptr;
	if (group_id <= 0 || msg_id.empty()) {
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
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"WHERE m.group_id = ? AND m.msg_id = ? LIMIT 1"));
		pstmt->setInt64(1, group_id);
		pstmt->setString(2, msg_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}

		auto one = std::make_shared<GroupMessageInfo>();
		one->msg_id = res->getString("msg_id");
		one->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
		one->from_uid = res->getInt("from_uid");
		one->msg_type = res->getString("msg_type");
		one->content = res->getString("content");
		one->mentions_json = res->isNull("mentions_json") ? "[]" : res->getString("mentions_json");
		one->reply_to_server_msg_id = res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
		one->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
		one->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
		one->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
		one->created_at = static_cast<int64_t>(std::stoll(res->getString("created_at")));
		one->server_msg_id = static_cast<int64_t>(std::stoll(res->getString("server_msg_id")));
		one->group_seq = static_cast<int64_t>(std::stoll(res->getString("group_seq")));
		one->file_name = res->isNull("file_name") ? "" : res->getString("file_name");
		one->mime = res->isNull("mime") ? "" : res->getString("mime");
		one->size = res->isNull("size") ? 0 : res->getInt("size");
		message = one;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpsertDialogDraft(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const std::string& draft_text) {
	if (owner_uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		return false;
	}

	const int normalized_peer_uid = (dialog_type == "private") ? peer_uid : 0;
	const int64_t normalized_group_id = (dialog_type == "group") ? group_id : 0;
	if ((dialog_type == "private" && normalized_peer_uid <= 0)
		|| (dialog_type == "group" && normalized_group_id <= 0)) {
		return false;
	}

	std::string normalized_draft = draft_text;
	if (normalized_draft.size() > 2000) {
		normalized_draft.resize(2000);
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
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES(?, ?, ?, ?, 0, ?, 0, '', 0, 0) "
				"ON DUPLICATE KEY UPDATE draft_text = VALUES(draft_text), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, owner_uid);
		pstmt->setString(2, dialog_type);
		pstmt->setInt(3, normalized_peer_uid);
		pstmt->setInt64(4, normalized_group_id);
		pstmt->setString(5, normalized_draft);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpsertDialogPinned(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const int& pinned_rank) {
	if (owner_uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		return false;
	}
	const int normalized_peer_uid = (dialog_type == "private") ? peer_uid : 0;
	const int64_t normalized_group_id = (dialog_type == "group") ? group_id : 0;
	if ((dialog_type == "private" && normalized_peer_uid <= 0)
		|| (dialog_type == "group" && normalized_group_id <= 0)) {
		return false;
	}
	const int normalized_pinned_rank = std::max(0, pinned_rank);

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
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES(?, ?, ?, ?, ?, '', 0, '', 0, 0) "
				"ON DUPLICATE KEY UPDATE pinned_rank = VALUES(pinned_rank), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, owner_uid);
		pstmt->setString(2, dialog_type);
		pstmt->setInt(3, normalized_peer_uid);
		pstmt->setInt64(4, normalized_group_id);
		pstmt->setInt(5, normalized_pinned_rank);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpsertDialogMuteState(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const int& mute_state) {
	if (owner_uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		return false;
	}
	const int normalized_peer_uid = (dialog_type == "private") ? peer_uid : 0;
	const int64_t normalized_group_id = (dialog_type == "group") ? group_id : 0;
	if ((dialog_type == "private" && normalized_peer_uid <= 0)
		|| (dialog_type == "group" && normalized_group_id <= 0)) {
		return false;
	}
	const int normalized_mute_state = mute_state > 0 ? 1 : 0;

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
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES(?, ?, ?, ?, 0, '', ?, '', 0, 0) "
				"ON DUPLICATE KEY UPDATE mute_state = VALUES(mute_state), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, owner_uid);
		pstmt->setString(2, dialog_type);
		pstmt->setInt(3, normalized_peer_uid);
		pstmt->setInt64(4, normalized_group_id);
		pstmt->setInt(5, normalized_mute_state);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

std::string MysqlDao::GenerateRandomGroupCode() {
	static thread_local std::mt19937_64 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(100000000, 999999999);
	return "g" + std::to_string(dist(rng));
}

bool MysqlDao::EnsureDialogMetaSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_dialog ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"owner_uid INT NOT NULL,"
			"dialog_type VARCHAR(16) NOT NULL,"
			"peer_uid INT NOT NULL DEFAULT 0,"
			"group_id BIGINT NOT NULL DEFAULT 0,"
			"pinned_rank INT NOT NULL DEFAULT 0,"
			"draft_text VARCHAR(2000) NOT NULL DEFAULT '',"
			"mute_state TINYINT NOT NULL DEFAULT 0,"
			"last_msg_preview VARCHAR(255) NOT NULL DEFAULT '',"
			"last_msg_ts BIGINT NOT NULL DEFAULT 0,"
			"unread_count INT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_owner_dialog(owner_uid, dialog_type, peer_uid, group_id),"
			"KEY idx_owner(owner_uid),"
			"KEY idx_owner_type(owner_uid, dialog_type)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN mute_state TINYINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN last_msg_preview VARCHAR(255) NOT NULL DEFAULT ''");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN last_msg_ts BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN unread_count INT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog MODIFY COLUMN draft_text VARCHAR(2000) NOT NULL DEFAULT ''");
		}
		catch (sql::SQLException&) {
		}
		ExecuteIgnoreSql(stmt.get(), "CREATE UNIQUE INDEX uq_owner_dialog ON chat_dialog(owner_uid, dialog_type, peer_uid, group_id)");
		ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_owner_type ON chat_dialog(owner_uid, dialog_type)");
		ExecuteIgnoreSql(stmt.get(), "DROP INDEX uq_chat_dialog_owner_dialog ON chat_dialog");
		ExecuteIgnoreSql(stmt.get(), "DROP INDEX idx_chat_dialog_owner_type ON chat_dialog");

		// Keep old table for migration compatibility with historical deployments.
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_dialog_meta ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"owner_uid INT NOT NULL,"
			"dialog_type VARCHAR(16) NOT NULL,"
			"peer_uid INT NOT NULL DEFAULT 0,"
			"group_id BIGINT NOT NULL DEFAULT 0,"
			"pinned_rank INT NOT NULL DEFAULT 0,"
			"draft_text VARCHAR(2000) NOT NULL DEFAULT '',"
			"mute_state TINYINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_owner_dialog(owner_uid, dialog_type, peer_uid, group_id),"
			"KEY idx_owner(owner_uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_dialog_meta ADD COLUMN mute_state TINYINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog_meta MODIFY COLUMN draft_text VARCHAR(2000) NOT NULL DEFAULT ''");
		}
		catch (sql::SQLException&) {
		}
		stmt->execute("INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
			"SELECT owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, '', 0, 0 "
			"FROM chat_dialog_meta "
			"ON DUPLICATE KEY UPDATE "
			"pinned_rank = CASE WHEN chat_dialog.pinned_rank = 0 THEN VALUES(pinned_rank) ELSE chat_dialog.pinned_rank END, "
			"draft_text = CASE WHEN chat_dialog.draft_text = '' THEN VALUES(draft_text) ELSE chat_dialog.draft_text END, "
			"mute_state = CASE WHEN chat_dialog.mute_state = 0 THEN VALUES(mute_state) ELSE chat_dialog.mute_state END");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureDialogMetaSchema SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::EnsurePrivateReadStateSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_private_read_state ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"uid INT NOT NULL,"
			"peer_uid INT NOT NULL,"
			"read_ts BIGINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_uid_peer(uid, peer_uid),"
			"KEY idx_peer_uid(peer_uid, uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_private_read_state ADD COLUMN read_ts BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsurePrivateReadStateSchema SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::EnsureGroupReadStateSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_read_state ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"uid INT NOT NULL,"
			"group_id BIGINT NOT NULL,"
			"read_ts BIGINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_uid_group(uid, group_id),"
			"KEY idx_group_uid(group_id, uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_group_read_state ADD COLUMN read_ts BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureGroupReadStateSchema SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::EnsureGroupMessageOrderSchema() {
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
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN server_msg_id BIGINT NOT NULL AUTO_INCREMENT UNIQUE");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN group_seq BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN reply_to_server_msg_id BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN forward_meta_json TEXT");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN edited_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN deleted_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN reply_to_server_msg_id BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN forward_meta_json TEXT");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN edited_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN deleted_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_group_seq ON chat_group_msg(group_id, group_seq)");

		stmt->execute("CREATE TEMPORARY TABLE IF NOT EXISTS tmp_group_seq_backfill ("
			"msg_id VARCHAR(64) PRIMARY KEY,"
			"group_seq BIGINT NOT NULL"
			") ENGINE=InnoDB");
		stmt->execute("TRUNCATE TABLE tmp_group_seq_backfill");
		stmt->execute("SET @memochat_prev_gid := -1, @memochat_seq := 0");
		stmt->execute(
			"INSERT INTO tmp_group_seq_backfill(msg_id, group_seq) "
			"SELECT ranked.msg_id, ranked.group_seq "
			"FROM ("
			"SELECT ordered.msg_id, "
			"(@memochat_seq := IF(@memochat_prev_gid = ordered.group_id, @memochat_seq + 1, 1)) AS group_seq, "
			"(@memochat_prev_gid := ordered.group_id) AS prev_gid "
			"FROM (SELECT msg_id, group_id FROM chat_group_msg ORDER BY group_id ASC, created_at ASC, msg_id ASC) ordered"
			") ranked");
		stmt->execute(
			"UPDATE chat_group_msg m "
			"JOIN tmp_group_seq_backfill t ON t.msg_id = m.msg_id "
			"SET m.group_seq = t.group_seq");
		stmt->execute("DROP TEMPORARY TABLE IF EXISTS tmp_group_seq_backfill");

		ExecuteIgnoreSql(stmt.get(), "DROP INDEX idx_chat_group_msg_group_seq ON chat_group_msg");
		ExecuteIgnoreSql(stmt.get(), "CREATE UNIQUE INDEX uk_chat_group_msg_group_seq ON chat_group_msg(group_id, group_seq)");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureGroupMessageOrderSchema SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::EnsureGroupPermissionSchemaAndBackfill() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_admin_permission ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"group_id BIGINT NOT NULL,"
			"uid INT NOT NULL,"
			"permission_bits BIGINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_group_uid(group_id, uid),"
			"KEY idx_uid(uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_group_admin_permission ADD COLUMN permission_bits BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}

		con->_con->setAutoCommit(false);
		stmt->execute("DELETE p FROM chat_group_admin_permission p "
			"LEFT JOIN chat_group_member m ON p.group_id = m.group_id AND p.uid = m.uid "
			"WHERE m.id IS NULL OR m.status <> 1 OR m.role < 2");

		std::unique_ptr<sql::PreparedStatement> upsert_owner(
			con->_con->prepareStatement(
				"INSERT INTO chat_group_admin_permission(group_id, uid, permission_bits) "
				"SELECT group_id, uid, ? FROM chat_group_member WHERE status = 1 AND role = 3 "
				"ON DUPLICATE KEY UPDATE permission_bits = VALUES(permission_bits), updated_at = CURRENT_TIMESTAMP"));
		upsert_owner->setInt64(1, kOwnerPermBits);
		upsert_owner->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> upsert_admin(
			con->_con->prepareStatement(
				"INSERT INTO chat_group_admin_permission(group_id, uid, permission_bits) "
				"SELECT group_id, uid, ? FROM chat_group_member WHERE status = 1 AND role = 2 "
				"ON DUPLICATE KEY UPDATE permission_bits = IF(permission_bits <= 0, VALUES(permission_bits), permission_bits), updated_at = CURRENT_TIMESTAMP"));
		upsert_admin->setInt64(1, kDefaultAdminPermBits);
		upsert_admin->executeUpdate();

		con->_con->commit();
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
		std::cerr << "EnsureGroupPermissionSchemaAndBackfill SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::EnsureGroupCodeSchemaAndBackfill() {
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
			stmt->execute("ALTER TABLE chat_group ADD COLUMN group_code VARCHAR(10) NULL");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group ADD COLUMN icon VARCHAR(512) NULL");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("CREATE UNIQUE INDEX uk_chat_group_code ON chat_group(group_code)");
		}
		catch (sql::SQLException&) {
		}

		std::unique_ptr<sql::PreparedStatement> query(
			con->_con->prepareStatement("SELECT group_id, group_code FROM chat_group ORDER BY group_id ASC"));
		std::unique_ptr<sql::ResultSet> rows(query->executeQuery());

		struct GroupRow {
			int64_t group_id = 0;
			std::string group_code;
		};
		std::vector<GroupRow> all_rows;
		std::unordered_set<std::string> used_codes;
		int duplicate_fixed = 0;

		while (rows->next()) {
			GroupRow row;
			row.group_id = static_cast<int64_t>(std::stoll(rows->getString("group_id")));
			row.group_code = rows->isNull("group_code") ? "" : rows->getString("group_code");
			if (IsValidGroupCode(row.group_code)) {
				if (used_codes.find(row.group_code) == used_codes.end()) {
					used_codes.insert(row.group_code);
				}
				else {
					row.group_code.clear();
					++duplicate_fixed;
				}
			}
			else {
				row.group_code.clear();
			}
			all_rows.push_back(row);
		}

		int backfilled = 0;
		int retries = 0;
		int owner_role_fixed = 0;
		int demoted_extra_owner = 0;
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> update_stmt(
			con->_con->prepareStatement("UPDATE chat_group SET group_code = ? WHERE group_id = ?"));

		for (auto& row : all_rows) {
			if (!row.group_code.empty()) {
				continue;
			}

			std::string new_code;
			for (int i = 0; i < 20; ++i) {
				const std::string candidate = GenerateRandomGroupCode();
				if (used_codes.find(candidate) == used_codes.end()) {
					new_code = candidate;
					break;
				}
				++retries;
			}

			if (new_code.empty()) {
				con->_con->rollback();
				return false;
			}

			update_stmt->setString(1, new_code);
			update_stmt->setInt64(2, row.group_id);
			update_stmt->executeUpdate();
			used_codes.insert(new_code);
			++backfilled;
		}

		std::unique_ptr<sql::PreparedStatement> ensure_owner_stmt(
			con->_con->prepareStatement("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
				"VALUES(?, ?, 3, 0, 0, 1) "
				"ON DUPLICATE KEY UPDATE role = 3, status = 1, mute_until = 0, updated_at = CURRENT_TIMESTAMP"));
		std::unique_ptr<sql::PreparedStatement> demote_stmt(
			con->_con->prepareStatement("UPDATE chat_group_member SET role = 2, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND uid <> ? AND role = 3 AND status = 1"));
		for (const auto& row : all_rows) {
			std::unique_ptr<sql::PreparedStatement> owner_query(
				con->_con->prepareStatement("SELECT owner_uid FROM chat_group WHERE group_id = ? LIMIT 1"));
			owner_query->setInt64(1, row.group_id);
			std::unique_ptr<sql::ResultSet> owner_res(owner_query->executeQuery());
			if (!owner_res->next()) {
				continue;
			}
			const int owner_uid = owner_res->getInt("owner_uid");
			if (owner_uid <= 0) {
				continue;
			}

			ensure_owner_stmt->setInt64(1, row.group_id);
			ensure_owner_stmt->setInt(2, owner_uid);
			owner_role_fixed += ensure_owner_stmt->executeUpdate();

			demote_stmt->setInt64(1, row.group_id);
			demote_stmt->setInt(2, owner_uid);
			demoted_extra_owner += demote_stmt->executeUpdate();
		}

		con->_con->commit();
		std::cout << "[ChatServer] group_code backfill done. total=" << all_rows.size()
			<< " backfilled=" << backfilled
			<< " duplicate_fixed=" << duplicate_fixed
			<< " retries=" << retries
			<< " owner_role_fixed=" << owner_role_fixed
			<< " demoted_extra_owner=" << demoted_extra_owner << std::endl;
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
		std::cerr << "EnsureGroupCodeSchemaAndBackfill SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::GetPendingGroupApplyForReviewer(const int& reviewer_uid, std::vector<std::shared_ptr<GroupApplyInfo>>& applies, int limit) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		const int final_limit = std::max(1, std::min(limit, 100));
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT a.apply_id, a.group_id, a.applicant_uid, a.inviter_uid, a.type, a.status, a.reason, a.reviewer_uid "
				"FROM chat_group_apply a "
				"JOIN chat_group_member m ON a.group_id = m.group_id "
				"LEFT JOIN chat_group_admin_permission p ON p.group_id = m.group_id AND p.uid = m.uid "
				"WHERE a.status = 0 AND m.uid = ? AND m.status = 1 AND "
				"(m.role = 3 OR ((COALESCE(p.permission_bits, ?) & ?) <> 0)) "
				"ORDER BY a.created_at DESC LIMIT ?"));
		pstmt->setInt(1, reviewer_uid);
		pstmt->setInt64(2, kDefaultAdminPermBits);
		pstmt->setInt64(3, kPermInviteUsers);
		pstmt->setInt(4, final_limit);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupApplyInfo>();
			info->apply_id = static_cast<int64_t>(std::stoll(res->getString("apply_id")));
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->applicant_uid = res->getInt("applicant_uid");
			info->inviter_uid = res->getInt("inviter_uid");
			info->type = (res->getInt("type") == 1) ? "invite" : "apply";
			info->status = res->getInt("status");
			info->reason = res->getString("reason");
			info->reviewer_uid = res->getInt("reviewer_uid");
			applies.push_back(info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
