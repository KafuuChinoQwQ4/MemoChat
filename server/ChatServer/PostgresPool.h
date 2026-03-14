#pragma once

#include "const.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace sql {
class SQLException final : public std::exception {
public:
	explicit SQLException(std::string message = "legacy MySQL path disabled") noexcept
		: message_(std::move(message)) {}

	const char* what() const noexcept override { return message_.c_str(); }
	int getErrorCode() const noexcept { return -1; }
	std::string getSQLState() const { return "HY000"; }

private:
	std::string message_;
};

class ResultSet {
public:
	bool next() { return false; }
	std::string getString(const std::string&) const { return ""; }
	int getInt(const std::string&) const { return 0; }
	int64_t getInt64(const std::string&) const { return 0; }
	bool isNull(const std::string&) const { return true; }
};

class PreparedStatement {
public:
	void setInt(int, int) {}
	void setInt64(int, int64_t) {}
	void setString(int, const std::string&) {}
	ResultSet* executeQuery() { return new ResultSet(); }
	int executeUpdate() { return 0; }
	bool execute() { return false; }
};

class Statement {
public:
	ResultSet* executeQuery(const std::string&) { return new ResultSet(); }
	bool execute(const std::string&) { return false; }
};

class Connection {
public:
	void setSchema(const std::string&) {}
	void setAutoCommit(bool) {}
	void rollback() {}
	void commit() {}
	PreparedStatement* prepareStatement(const std::string&) {
		return new PreparedStatement();
	}
	Statement* createStatement() { return new Statement(); }
};

namespace mysql {
class MySQL_Driver {
public:
	Connection* connect(const std::string&, const std::string&, const std::string&) {
		return new Connection();
	}
};

inline MySQL_Driver* get_mysql_driver_instance() {
	static MySQL_Driver driver;
	return &driver;
}
}  // namespace mysql
}  // namespace sql

class PooledSqlConnection {
public:
	PooledSqlConnection(sql::Connection* con, int64_t lasttime);

	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class PostgresPool {
public:
	PostgresPool(const std::string& url, const std::string& user, const std::string& pass,
		const std::string& schema, int poolSize);
	~PostgresPool();

	std::unique_ptr<PooledSqlConnection> getConnection();
	void returnConnection(std::unique_ptr<PooledSqlConnection> con);
	void Close();

private:
	void checkConnection();
	void checkConnectionPro();
	bool reconnect(long long timestamp);

	std::string url_;
	std::string user_;
	std::string pass_;
	std::string schema_;
	int poolSize_;
	std::queue<std::unique_ptr<PooledSqlConnection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;
	std::atomic<int> _fail_count;
};

using SqlConnection = PooledSqlConnection;
