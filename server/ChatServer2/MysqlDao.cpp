#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <set>
#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <limits>

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
				"name VARCHAR(64) NOT NULL,"
				"owner_uid INT NOT NULL,"
				"announcement TEXT DEFAULT '',"
				"member_limit INT NOT NULL DEFAULT 200,"
				"is_all_muted TINYINT NOT NULL DEFAULT 0,"
				"status TINYINT NOT NULL DEFAULT 1,"
				"created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
				"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
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
				"group_id BIGINT NOT NULL,"
				"from_uid INT NOT NULL,"
				"msg_type VARCHAR(16) NOT NULL,"
				"content TEXT NOT NULL,"
				"mentions_json TEXT DEFAULT '',"
				"created_at BIGINT NOT NULL,"
				"KEY idx_group_created(group_id, created_at)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_msg_ext ("
				"msg_id VARCHAR(64) PRIMARY KEY,"
				"file_name VARCHAR(255) DEFAULT '',"
				"mime VARCHAR(127) DEFAULT '',"
				"size INT NOT NULL DEFAULT 0"
				") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
			pool_->returnConnection(std::move(con));
		}
	}
	catch (sql::SQLException& e) {
		std::cerr << "init tag tables failed: " << e.what() << std::endl;
	}
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
		// ׼�����ô洢����
		std::unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// �����������
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// ����PreparedStatement��ֱ��֧��ע�����������������Ҫʹ�ûỰ������������������ȡ���������ֵ

		  // ִ�д洢����
		stmt->execute();
		// ����洢���������˻Ự��������������ʽ��ȡ���������ֵ�������������ִ��SELECT��ѯ����ȡ����
	   // ���磬����洢����������һ���Ự����@result���洢������������������ȡ��
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

		// ׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

		// �󶨲���
		pstmt->setString(1, name);

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// ���������
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

		// ׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		// �󶨲���
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		// ִ�и���
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
		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name); // ��username�滻Ϊ��Ҫ��ѯ���û���

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// ���������
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// �����ѯ��������
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.name = name;
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
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
		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
			"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
		pstmt->setInt(1, from); // from id
		pstmt->setInt(2, to);
		// ִ�и���
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
		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
			"WHERE from_uid = ? AND to_uid = ?"));
		//������������ʱfrom����֤ʱto
		pstmt->setInt(1, to); // from id
		pstmt->setInt(2, from);
		// ִ�и���
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

		//��ʼ����
		con->_con->setAutoCommit(false);

		// ׼����һ��SQL���, ������֤����������
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
			));
		//������������ʱfrom����֤ʱto
		pstmt->setInt(1, from); // from id
		pstmt->setInt(2, to);
		pstmt->setString(3, back_name);
		// ִ�и���
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			con->_con->rollback();
			return false;
		}

		//׼���ڶ���SQL��䣬�������뷽��������
		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
		));
		//������������ʱfrom����֤ʱto
		pstmt2->setInt(1, to); // from id
		pstmt2->setInt(2, from);
		pstmt2->setString(3, "");
		// ִ�и���
		int rowAffected2 = pstmt2->executeUpdate();
		if (rowAffected2 < 0) {
			con->_con->rollback();
			return false;
		}

		// �ύ����
		con->_con->commit();
		std::cout << "addfriend insert friends success" << std::endl;

		return true;
	}
	catch (sql::SQLException& e) {
		// ����������󣬻ع�����
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
		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid); // ��uid�滻Ϊ��Ҫ��ѯ��uid

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		// ���������
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name= res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->icon = res->getString("icon");
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
		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name); // ��uid�滻Ϊ��Ҫ��ѯ��uid

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		// ���������
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
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


bool MysqlDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});


		try {
		// ׼��SQL���, ������ʼid���������������б�
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
				"user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
			"and apply.id > ? order by apply.id ASC LIMIT ? "));

		pstmt->setInt(1, touid); // ��uid�滻Ϊ��Ҫ��ѯ��uid
		pstmt->setInt(2, begin); // ��ʼid
		pstmt->setInt(3, limit); //ƫ����
		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// ���������
		while (res->next()) {	
			auto name = res->getString("name");
			auto uid = res->getInt("from_uid");
			auto status = res->getInt("status");
			auto nick = res->getString("nick");
			auto sex = res->getInt("sex");
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
			apply_ptr->_labels = GetApplyTags(uid, touid);
			applyList.push_back(apply_ptr);
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
		// ׼��SQL���, ������ʼid���������������б�
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select * from friend where self_id = ? "));

		pstmt->setInt(1, self_id); // ��uid�滻Ϊ��Ҫ��ѯ��uid
	
		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// ���������
		while (res->next()) {		
			auto friend_id = res->getInt("friend_id");
			auto back = res->getString("back");
			//��һ�β�ѯfriend_id��Ӧ����Ϣ
			auto user_info = GetUser(friend_id);
			if (user_info == nullptr) {
				continue;
			}

			user_info->back = back;
			user_info->labels = GetFriendTags(self_id, friend_id);
			user_info_list.push_back(user_info);
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
	const int& member_limit, const std::vector<int>& initial_members, int64_t& out_group_id) {
	out_group_id = 0;
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

		std::unique_ptr<sql::PreparedStatement> ins_group(
			con->_con->prepareStatement("INSERT INTO chat_group(name, owner_uid, announcement, member_limit, is_all_muted, status) "
				"VALUES(?,?,?,?,0,1)"));
		ins_group->setString(1, name);
		ins_group->setInt(2, owner_uid);
		ins_group->setString(3, announcement);
		ins_group->setInt(4, final_limit);
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
				"SELECT g.group_id, g.name, g.owner_uid, g.announcement, g.member_limit, g.is_all_muted, g.status, m.role, "
				"(SELECT COUNT(1) FROM chat_group_member gm WHERE gm.group_id = g.group_id AND gm.status = 1) AS member_count "
				"FROM chat_group_member m JOIN chat_group g ON m.group_id = g.group_id "
				"WHERE m.uid = ? AND m.status = 1 AND g.status = 1 ORDER BY g.updated_at DESC"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupInfo>();
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->name = res->getString("name");
			info->owner_uid = res->getInt("owner_uid");
			info->announcement = res->getString("announcement");
			info->member_limit = res->getInt("member_limit");
			info->is_all_muted = res->getInt("is_all_muted");
			info->status = res->getInt("status");
			info->role = res->getInt("role");
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
				"SELECT m.group_id, m.uid, m.role, m.mute_until, m.status, u.name, u.nick, u.icon "
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
	int role = 0;
	if (!GetUserRoleInGroup(group_id, inviter_uid, role) || role < 2) {
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

		int reviewer_role = 0;
		if (!GetUserRoleInGroup(found->group_id, reviewer_uid, reviewer_role) || reviewer_role < 2) {
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

bool MysqlDao::SaveGroupMessage(const GroupMessageInfo& msg) {
	if (msg.msg_id.empty() || msg.group_id <= 0 || msg.from_uid <= 0) {
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
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> ins_msg(
			con->_con->prepareStatement("INSERT INTO chat_group_msg(msg_id, group_id, from_uid, msg_type, content, mentions_json, created_at) "
				"VALUES(?,?,?,?,?,?,?)"));
		ins_msg->setString(1, msg.msg_id);
		ins_msg->setInt64(2, msg.group_id);
		ins_msg->setInt(3, msg.from_uid);
		ins_msg->setString(4, msg.msg_type);
		ins_msg->setString(5, msg.content);
		ins_msg->setString(6, msg.mentions_json);
		ins_msg->setInt64(7, msg.created_at);
		if (ins_msg->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
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

bool MysqlDao::GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int& limit,
	std::vector<std::shared_ptr<GroupMessageInfo>>& messages) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		const int final_limit = std::max(1, std::min(limit, 50));
		const int64_t final_before = (before_ts <= 0) ? std::numeric_limits<int64_t>::max() : before_ts;
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT m.msg_id, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, m.created_at, "
				"e.file_name, e.mime, e.size "
				"FROM chat_group_msg m LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"WHERE m.group_id = ? AND m.created_at < ? ORDER BY m.created_at DESC LIMIT ?"));
		pstmt->setInt64(1, group_id);
		pstmt->setInt64(2, final_before);
		pstmt->setInt(3, final_limit);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupMessageInfo>();
			info->msg_id = res->getString("msg_id");
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->from_uid = res->getInt("from_uid");
			info->msg_type = res->getString("msg_type");
			info->content = res->getString("content");
			info->mentions_json = res->getString("mentions_json");
			info->created_at = static_cast<int64_t>(std::stoll(res->getString("created_at")));
			info->file_name = res->isNull("file_name") ? "" : res->getString("file_name");
			info->mime = res->isNull("mime") ? "" : res->getString("mime");
			info->size = res->isNull("size") ? 0 : res->getInt("size");
			messages.push_back(info);
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
	int role = 0;
	if (!GetUserRoleInGroup(group_id, operator_uid, role) || role < 2) {
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

bool MysqlDao::SetGroupAdmin(const int64_t& group_id, const int& operator_uid, const int& target_uid, const bool& is_admin) {
	int role = 0;
	if (!GetUserRoleInGroup(group_id, operator_uid, role) || role != 3) {
		return false;
	}
	int target_role = 0;
	if (!GetUserRoleInGroup(group_id, target_uid, target_role) || target_role == 3) {
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
			con->_con->prepareStatement("UPDATE chat_group_member SET role = ?, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND uid = ? AND status = 1"));
		pstmt->setInt(1, is_admin ? 2 : 1);
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

bool MysqlDao::MuteGroupMember(const int64_t& group_id, const int& operator_uid, const int& target_uid, const int64_t& mute_until) {
	int op_role = 0;
	if (!GetUserRoleInGroup(group_id, operator_uid, op_role) || op_role < 2) {
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
	if (!GetUserRoleInGroup(group_id, operator_uid, op_role) || op_role < 2) {
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
		return pstmt->executeUpdate() > 0;
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
		return pstmt->executeUpdate() > 0;
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
			con->_con->prepareStatement("SELECT group_id, name, owner_uid, announcement, member_limit, is_all_muted, status "
				"FROM chat_group WHERE group_id = ? LIMIT 1"));
		pstmt->setInt64(1, group_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}
		auto info = std::make_shared<GroupInfo>();
		info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
		info->name = res->getString("name");
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
				"WHERE a.status = 0 AND m.uid = ? AND m.status = 1 AND m.role >= 2 "
				"ORDER BY a.created_at DESC LIMIT ?"));
		pstmt->setInt(1, reviewer_uid);
		pstmt->setInt(2, final_limit);
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
