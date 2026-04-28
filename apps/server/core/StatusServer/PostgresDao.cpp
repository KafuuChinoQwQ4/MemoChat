#include "PostgresDao.h"
#include "ConfigMgr.h"

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
	return "host=" + host +
		" port=" + port +
		" user=" + user +
		" password=" + pwd +
		" dbname=" + database +
		" sslmode=" + (sslmode.empty() ? "disable" : sslmode) +
		" options=-csearch_path=" + schema + ",public";
}
}

PostgresDao::PostgresDao()
{
	pool_.reset(new PostgresPool(BuildConnectionString(), 5));
}

PostgresDao::~PostgresDao(){
	pool_->Close();
}

int PostgresDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return -1;
		}
		pqxx::work txn(*con->_con);
		const auto exists = txn.exec1(
			"SELECT COUNT(1) FROM \"user\" WHERE name = " + txn.quote(name) + " OR email = " + txn.quote(email))[0].as<int>();
		if (exists > 0) {
			txn.commit();
			pool_->returnConnection(std::move(con));
			return 0;
		}

		const auto uid = txn.exec1("SELECT COALESCE(MAX(id), 1000) + 1 FROM user_id")[0].as<int>();
		txn.exec("DELETE FROM user_id");
		txn.exec0("INSERT INTO user_id(id) VALUES (" + txn.quote(uid) + ")");
		txn.exec0(
			"INSERT INTO \"user\"(uid, name, email, pwd, nick, \"desc\", sex, icon) VALUES (" +
			txn.quote(uid) + ", " +
			txn.quote(name) + ", " +
			txn.quote(email) + ", " +
			txn.quote(pwd) + ", " +
			txn.quote(name) + ", '', 0, ':/res/head_1.jpg')");
		txn.commit();
		pool_->returnConnection(std::move(con));
		return uid;
	}
	catch (const std::exception& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return -1;
	}
}

bool PostgresDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}
		pqxx::read_transaction txn(*con->_con);
		auto rows = txn.exec_params("SELECT email FROM \"user\" WHERE name = $1 LIMIT 1", name);
		if (rows.empty()) {
			pool_->returnConnection(std::move(con));
			return true;
		}
		const auto stored_email = rows[0]["email"].c_str() ? rows[0]["email"].c_str() : "";
		pool_->returnConnection(std::move(con));
		return email == stored_email;
	}
	catch (const std::exception& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}
		pqxx::work txn(*con->_con);
		auto result = txn.exec_params("UPDATE \"user\" SET pwd = $1 WHERE name = $2", newpwd, name);
		const auto updateCount = result.affected_rows();
		txn.commit();
		pool_->returnConnection(std::move(con));
		return updateCount > 0;
	}
	catch (const std::exception& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		pqxx::read_transaction txn(*con->_con);
		auto rows = txn.exec_params(
			"SELECT uid, email, pwd, name FROM \"user\" WHERE name = $1 OR email = $1 ORDER BY uid ASC LIMIT 1",
			name);
		if (rows.empty()) {
			return false;
		}
		const auto& row = rows[0];
		const std::string origin_pwd = row["pwd"].c_str() ? row["pwd"].c_str() : "";
		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.name = row["name"].c_str() ? row["name"].c_str() : "";
		userInfo.email = row["email"].c_str() ? row["email"].c_str() : "";
		userInfo.uid = row["uid"].as<int>();
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}


