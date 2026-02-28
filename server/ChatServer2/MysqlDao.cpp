#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <set>

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
