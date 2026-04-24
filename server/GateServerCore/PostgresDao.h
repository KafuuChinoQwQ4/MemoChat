#pragma once
#include "const.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pqxx/pqxx>
#include <thread>
#include <cstdint>

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
		: connection_string_(connection_string), poolSize_(poolSize), b_stop_(false) {
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
		if (closed_.load(std::memory_order_acquire)) {
			return nullptr;
		}
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] { 
			if (b_stop_) {
				return true;
			}		
			return !pool_.empty(); });
		if (b_stop_ || closed_.load(std::memory_order_acquire)) {
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
		closed_.store(true, std::memory_order_release);
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
	std::atomic<bool> closed_{false};
	std::thread _check_thread;
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string user_id;
	std::string email;
	std::string nick;
	std::string icon;
	std::string desc;
	int sex = 0;
};

struct CallUserProfile {
	int uid = 0;
	std::string user_id;
	std::string name;
	std::string nick;
	std::string icon;
};

struct CallSessionInfo {
	std::string call_id;
	std::string room_name;
	std::string call_type;
	int caller_uid = 0;
	int callee_uid = 0;
	std::string state;
	int64_t started_at_ms = 0;
	int64_t accepted_at_ms = 0;
	int64_t ended_at_ms = 0;
	int64_t expires_at_ms = 0;
	int duration_sec = 0;
	std::string reason;
	std::string trace_id;
	int64_t updated_at_ms = 0;
};

struct MediaAssetInfo {
	int64_t media_id = 0;
	std::string media_key;
	int owner_uid = 0;
	std::string media_type;
	std::string origin_file_name;
	std::string mime;
	int64_t size_bytes = 0;
	std::string storage_provider;
	std::string storage_path;
	int64_t created_at_ms = 0;
	int64_t deleted_at_ms = 0;
	int status = 1;
};

struct MomentInfo {
	int64_t moment_id = 0;
	int uid = 0;
	int visibility = 0;   // 0=public, 1=friends, 2=private
	std::string location;
	int64_t created_at = 0;
	int64_t deleted_at = 0;
	int like_count = 0;
	int comment_count = 0;
};

struct MomentLikeInfo {
	int64_t id = 0;
	int64_t moment_id = 0;
	int uid = 0;
	std::string user_nick;
	std::string user_icon;
	int64_t created_at = 0;
};

struct MomentCommentInfo {
	int64_t id = 0;
	int64_t moment_id = 0;
	int uid = 0;
	std::string user_nick;
	std::string user_icon;
	std::string content;
	int reply_uid = 0;
	std::string reply_nick;
	int64_t created_at = 0;
	int64_t deleted_at = 0;
};

class PostgresDao
{
public:
	PostgresDao();
	~PostgresDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	std::string GetUserPublicId(int uid);
	bool GetCallUserProfile(int uid, CallUserProfile& profile);
	bool IsFriend(int uid, int peer_uid);
	bool UpsertCallSession(const CallSessionInfo& session);
	bool GetCallSession(const std::string& call_id, CallSessionInfo& session);
	bool InsertMediaAsset(const MediaAssetInfo& asset);
	bool GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset);
	bool GetUserInfo(int uid, UserInfo& user_info);
	bool TestProcedure(const std::string& email, int& uid, string& name);

	// Moments operations
	bool AddMoment(const MomentInfo& moment);
	bool GetMomentsFeed(int viewer_uid, int64_t last_moment_id, int limit,
		std::vector<MomentInfo>& moments, bool& has_more);
	bool CanViewMoment(int viewer_uid, const MomentInfo& moment);
	bool GetMomentById(int64_t moment_id, MomentInfo& moment);
	bool DeleteMoment(int64_t moment_id, int uid);
	bool AddMomentLike(int64_t moment_id, int uid);
	bool RemoveMomentLike(int64_t moment_id, int uid);
	bool HasLikedMoment(int64_t moment_id, int uid);
	bool GetMomentLikes(int64_t moment_id, int limit,
		std::vector<MomentLikeInfo>& likes, bool& has_more);
	bool AddMomentComment(const MomentCommentInfo& comment);
	bool DeleteMomentComment(int64_t comment_id, int uid);
	bool GetMomentComments(int64_t moment_id, int64_t last_comment_id, int limit,
		std::vector<MomentCommentInfo>& comments, bool& has_more);

private:
	void WarmupAuthQueries();
	std::string GenerateUserPublicId();
	std::unique_ptr<PostgresPool> pool_;
};
