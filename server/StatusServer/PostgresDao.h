#pragma once
#include "const.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pqxx/pqxx>
#include <thread>

class PooledSqlConnection {
public:
	PooledSqlConnection(std::unique_ptr<pqxx::connection> con, int64_t lasttime)
		: _con(std::move(con)), _last_oper_time(lasttime) {}
	std::unique_ptr<pqxx::connection> _con;
	int64_t _last_oper_time;
};

class PostgresPool {
public:
	PostgresPool(const std::string& connection_string, int poolSize)
		: connection_string_(connection_string), poolSize_(poolSize), b_stop_(false){
		try {
			for (int i = 0; i < poolSize_; ++i) {
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<PooledSqlConnection>(std::make_unique<pqxx::connection>(connection_string_), timestamp));
			}

			_check_thread = 	std::thread([this]() {
				while (!b_stop_) {
					checkConnection();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
			});

			_check_thread.detach();
		}
		catch (const std::exception& e) {
			std::cout << "postgres pool init failed, error is " << e.what()<< std::endl;
		}
	}

	void checkConnection() {
		std::lock_guard<std::mutex> guard(mutex_);
		int poolsize = pool_.size();

		auto currentTime = std::chrono::system_clock::now().time_since_epoch();

		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());
			pool_.pop();
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
			});

			if (timestamp - con->_last_oper_time < 5) {
				continue;
			}
			
			try {
				pqxx::read_transaction txn(*con->_con);
				txn.exec("SELECT 1");
				txn.commit();
				con->_last_oper_time = timestamp;
			}
			catch (const std::exception& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				con->_con = std::make_unique<pqxx::connection>(connection_string_);
				con->_last_oper_time = timestamp;
			}
		}
	}

	std::unique_ptr<PooledSqlConnection> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { 
			if (b_stop_) {
				return true;
			}		
			return !pool_.empty(); });
		if (b_stop_) {
			return nullptr;
		}
		std::unique_ptr<PooledSqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void returnConnection(std::unique_ptr<PooledSqlConnection> con) {
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

	~PostgresPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string connection_string_;
	int poolSize_;
	std::queue<std::unique_ptr<PooledSqlConnection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

class PostgresDao
{
public:
	PostgresDao();
	~PostgresDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
private:
	std::unique_ptr<PostgresPool> pool_;
};


