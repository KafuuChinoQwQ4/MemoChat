#include "PostgresPool.h"

PooledSqlConnection::PooledSqlConnection(sql::Connection* con, int64_t lasttime)
	: _con(con), _last_oper_time(lasttime) {}

namespace {
int64_t CurrentUnixSeconds() {
	const auto now = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}
}

PostgresPool::PostgresPool(const std::string& url, const std::string& user, const std::string& pass,
	const std::string& schema, int poolSize)
	: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false), _fail_count(0) {
	for (int i = 0; i < poolSize_; ++i) {
		try {
			pool_.push(std::make_unique<PooledSqlConnection>(new sql::Connection(), CurrentUnixSeconds()));
		}
		catch (const std::exception&) {
			break;
		}
	}
}

void PostgresPool::checkConnectionPro() {}

bool PostgresPool::reconnect(long long timestamp) {
	std::lock_guard<std::mutex> guard(mutex_);
	if (b_stop_) {
		return false;
	}
	pool_.push(std::make_unique<PooledSqlConnection>(new sql::Connection(), timestamp));
	cond_.notify_one();
	return true;
}

void PostgresPool::checkConnection() {}

std::unique_ptr<PooledSqlConnection> PostgresPool::getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);
	if (!cond_.wait_for(lock, std::chrono::seconds(5), [this] {
		return b_stop_ || !pool_.empty();
	})) {
		return nullptr;
	}
	if (b_stop_) {
		return nullptr;
	}
	auto con = std::move(pool_.front());
	pool_.pop();
	return con;
}

void PostgresPool::returnConnection(std::unique_ptr<PooledSqlConnection> con) {
	if (!con) {
		return;
	}
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) {
		return;
	}
	con->_last_oper_time = CurrentUnixSeconds();
	pool_.push(std::move(con));
	cond_.notify_one();
}

void PostgresPool::Close() {
	b_stop_ = true;
	cond_.notify_all();
}

PostgresPool::~PostgresPool() {
	Close();
	std::lock_guard<std::mutex> lock(mutex_);
	while (!pool_.empty()) {
		pool_.pop();
	}
}
