#pragma once
#include "const.h"
#include <thread>
#include <memory>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include "AsioIOServicePool.h"
#include <queue>              // [新增] 必须包含
#include <mutex>              // [新增] 必须包含
#include <condition_variable> // [新增] 必须包含
#include <atomic>
#include <vector>

// 简单的连接池实现
class MySqlPool {
public:
	MySqlPool(std::string url, std::string user, std::string pass, std::string schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
		try {
			driver_ = sql::mysql::get_mysql_driver_instance();
			for (int i = 0; i < poolSize_; ++i) {
				sql::Connection* con = driver_->connect(url_, user_, pass_);
				con->setSchema(schema_);
				pool_.push(std::unique_ptr<sql::Connection>(con));
			}
		}
		catch (sql::SQLException& e) {
			std::cout << "mysql pool init failed: " << e.what() << std::endl;
		}
	}

	std::unique_ptr<sql::Connection> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !pool_.empty();
			});
		if (b_stop_) {
			return nullptr;
		}
		std::unique_ptr<sql::Connection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void returnConnection(std::unique_ptr<sql::Connection> con) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		pool_.push(std::move(con));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

	~MySqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string url_;
	std::string user_;
	std::string pass_;
	std::string schema_;
	int poolSize_;
	std::queue<std::unique_ptr<sql::Connection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	sql::mysql::MySQL_Driver* driver_;
};

class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& pwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, std::string& userInfo);
	bool AddFriendApply(const int& from, const int& to);
	bool AuthFriendApply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit);
	
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name); // [重点] 确保这个声明存在

private:
	std::unique_ptr<MySqlPool> pool_;
};