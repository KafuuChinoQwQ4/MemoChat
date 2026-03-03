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
	


		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
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
