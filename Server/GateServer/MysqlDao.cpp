#include "MysqlDao.h"
#include "ConfigMgr.h"

MysqlDao::MysqlDao()
{
	auto& cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return 0;
	}
	
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		if (con == nullptr) {
			return 0;
		}
		
		// 1. 先检查是否存在
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT 1 FROM user WHERE name = ? OR email = ?"));
		pstmt->setString(1, name);
		pstmt->setString(2, email);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		
		if (res->next()) {
			return 0; // 用户已存在
		}

		// 2. 插入新用户
		// 注意：这里使用了 user_id 调用存储过程 reg_user
		std::unique_ptr<sql::PreparedStatement> pstmt_reg(con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		pstmt_reg->setString(1, name);
		pstmt_reg->setString(2, email);
		pstmt_reg->setString(3, pwd);
		pstmt_reg->execute();

		std::unique_ptr<sql::Statement> stmtResult(con->createStatement());
		std::unique_ptr<sql::ResultSet> resResult(stmtResult->executeQuery("SELECT @result"));
		if (resResult->next()) {
			int uid = resResult->getInt(1); // 获取生成的 UID
			return uid;
		}
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		return 0;
	}
}

// [核心修复] 必须正确实现这个函数，GateServer 分布式登录逻辑强依赖它
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
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		
		if (res->next()) {
			auto user_ptr = std::make_shared<UserInfo>();
			user_ptr->uid = res->getInt("uid");
			user_ptr->name = res->getString("name");
			user_ptr->email = res->getString("email");
			user_ptr->pwd = res->getString("pwd");
			// user_ptr->icon = res->getString("icon"); // 如果数据库有icon字段
			return user_ptr;
		}
		return nullptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		return nullptr;
	}
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
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		
		if (res->next()) {
			auto user_ptr = std::make_shared<UserInfo>();
			user_ptr->uid = res->getInt("uid");
			user_ptr->name = res->getString("name");
			user_ptr->email = res->getString("email");
			user_ptr->pwd = res->getString("pwd");
			return user_ptr;
		}
		return nullptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		return nullptr;
	}
}

bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, std::string& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			if (pwd == res->getString("pwd")) {
				return true;
			}
		}
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT email FROM user WHERE name = ?"));
		pstmt->setString(1, name);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		if (res->next()) {
			if (email == res->getString("email")) {
				return true;
			}
		}
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
		pstmt->setString(1, newpwd);
		pstmt->setString(2, name);
		int updateCount = pstmt->executeUpdate();
		return updateCount > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		return false;
	}
}