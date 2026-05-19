#include "PostgresDao.h"
#include "db/PqxxCompat.h"
#include "PostgresPool.h"
#include <pqxx/pqxx>
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace {
constexpr int64_t kPermDeleteMessages = 1LL << 1;
}
bool PostgresDao::SaveGroupMessage(const GroupMessageInfo& msg, int64_t* out_server_msg_id, int64_t* out_group_seq, int64_t assigned_group_seq) {
	if (msg.msg_id.empty() || msg.group_id <= 0 || msg.from_uid <= 0) {
		return false;
	}
	if (out_server_msg_id) {
		*out_server_msg_id = 0;
	}
	if (out_group_seq) {
		*out_group_seq = 0;
	}

	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			const auto group_rows = txn.exec_params(
				"SELECT group_id FROM chat_group WHERE group_id = $1 AND status = 1 LIMIT 1 FOR UPDATE",
				msg.group_id);
			if (group_rows.empty()) {
				return false;
			}

			int64_t next_group_seq = assigned_group_seq;
			if (next_group_seq <= 0) {
				const auto seq_rows = txn.exec_params(
					"SELECT COALESCE(MAX(group_seq), 0) + 1 AS next_seq FROM chat_group_msg WHERE group_id = $1",
					msg.group_id);
				next_group_seq = seq_rows.empty() ? 1 : seq_rows[0]["next_seq"].as<int64_t>();
				if (next_group_seq <= 0) {
					next_group_seq = 1;
				}
			}

			const auto insert_rows = txn.exec_params(
				"INSERT INTO chat_group_msg(msg_id, group_id, group_seq, from_uid, msg_type, content, "
				"mentions_json, reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at) "
				"VALUES($1,$2,$3,$4,$5,$6,COALESCE(NULLIF($7, ''), '[]')::jsonb,$8,NULLIF($9, '')::jsonb,$10,$11,$12) "
				"ON CONFLICT (group_id, msg_id) DO UPDATE SET "
				"from_uid = EXCLUDED.from_uid, msg_type = EXCLUDED.msg_type, content = EXCLUDED.content, "
				"mentions_json = EXCLUDED.mentions_json, reply_to_server_msg_id = EXCLUDED.reply_to_server_msg_id, "
				"forward_meta_json = EXCLUDED.forward_meta_json, edited_at_ms = EXCLUDED.edited_at_ms, "
				"deleted_at_ms = EXCLUDED.deleted_at_ms, created_at = EXCLUDED.created_at "
				"RETURNING server_msg_id, group_seq",
				msg.msg_id,
				msg.group_id,
				next_group_seq,
				msg.from_uid,
				msg.msg_type,
				msg.content,
				msg.mentions_json,
				msg.reply_to_server_msg_id,
				msg.forward_meta_json,
				msg.edited_at_ms,
				msg.deleted_at_ms,
				msg.created_at);
			if (insert_rows.empty()) {
				return false;
			}

			const auto server_msg_id = insert_rows[0]["server_msg_id"].as<int64_t>();
			next_group_seq = insert_rows[0]["group_seq"].as<int64_t>();
			if (!msg.file_name.empty() || !msg.mime.empty() || msg.size > 0) {
				txn.exec_params0(
					"INSERT INTO chat_group_msg_ext(msg_id, file_name, mime, size) VALUES($1,$2,$3,$4) "
					"ON CONFLICT (msg_id) DO UPDATE SET file_name = EXCLUDED.file_name, mime = EXCLUDED.mime, size = EXCLUDED.size",
					msg.msg_id,
					msg.file_name,
					msg.mime,
					msg.size);
			}

			txn.commit();
			if (out_server_msg_id) {
				*out_server_msg_id = server_msg_id;
			}
			if (out_group_seq) {
				*out_group_seq = next_group_seq;
			}
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
			return false;
		}
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
		{
			std::unique_ptr<sql::PreparedStatement> lock_group_stmt(
				con->_con->prepareStatement("SELECT group_id FROM chat_group WHERE group_id = ? AND status = 1 LIMIT 1 FOR UPDATE"));
			lock_group_stmt->setInt64(1, msg.group_id);
			std::unique_ptr<sql::ResultSet> lock_group_res(lock_group_stmt->executeQuery());
			if (!lock_group_res->next()) {
				con->_con->rollback();
				return false;
			}
		}
		int64_t next_group_seq = assigned_group_seq;
		if (next_group_seq <= 0) {
			std::unique_ptr<sql::PreparedStatement> seq_stmt(
				con->_con->prepareStatement("SELECT COALESCE(MAX(group_seq), 0) + 1 AS next_seq "
					"FROM chat_group_msg WHERE group_id = ?"));
			seq_stmt->setInt64(1, msg.group_id);
			std::unique_ptr<sql::ResultSet> seq_res(seq_stmt->executeQuery());
			if (!seq_res->next()) {
				con->_con->rollback();
				return false;
			}
			next_group_seq = seq_res->getInt64("next_seq");
			if (next_group_seq <= 0) {
				next_group_seq = 1;
			}
		}

		std::unique_ptr<sql::PreparedStatement> ins_msg(
			con->_con->prepareStatement("INSERT INTO chat_group_msg(msg_id, group_id, group_seq, from_uid, msg_type, content, "
				"mentions_json, reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms, created_at) "
				"VALUES(?,?,?,?,?,?,?,?,?,?,?,?)"));
		ins_msg->setString(1, msg.msg_id);
		ins_msg->setInt64(2, msg.group_id);
		ins_msg->setInt64(3, next_group_seq);
		ins_msg->setInt(4, msg.from_uid);
		ins_msg->setString(5, msg.msg_type);
		ins_msg->setString(6, msg.content);
		ins_msg->setString(7, msg.mentions_json);
		ins_msg->setInt64(8, msg.reply_to_server_msg_id);
		ins_msg->setString(9, msg.forward_meta_json);
		ins_msg->setInt64(10, msg.edited_at_ms);
		ins_msg->setInt64(11, msg.deleted_at_ms);
		ins_msg->setInt64(12, msg.created_at);
		if (ins_msg->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		int64_t server_msg_id = 0;
		{
			std::unique_ptr<sql::Statement> id_stmt(con->_con->createStatement());
			std::unique_ptr<sql::ResultSet> id_res(id_stmt->executeQuery("SELECT LAST_INSERT_ID() AS server_msg_id"));
			if (id_res->next()) {
				server_msg_id = id_res->getInt64("server_msg_id");
			}
		}
		if (server_msg_id <= 0) {
			std::unique_ptr<sql::PreparedStatement> fallback_stmt(
				con->_con->prepareStatement("SELECT server_msg_id FROM chat_group_msg WHERE msg_id = ? LIMIT 1"));
			fallback_stmt->setString(1, msg.msg_id);
			std::unique_ptr<sql::ResultSet> fallback_res(fallback_stmt->executeQuery());
			if (fallback_res->next()) {
				server_msg_id = fallback_res->getInt64("server_msg_id");
			}
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
		if (out_server_msg_id) {
			*out_server_msg_id = server_msg_id;
		}
		if (out_group_seq) {
			*out_group_seq = next_group_seq;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::UpdateGroupMessageContent(const int64_t& group_id, const int& operator_uid, const std::string& msg_id,
	const std::string& content, int64_t edited_at_ms) {
	if (group_id <= 0 || operator_uid <= 0 || msg_id.empty() || content.empty()) {
		return false;
	}
	if (edited_at_ms <= 0) {
		edited_at_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			const auto rows = txn.exec_params(
				"SELECT from_uid FROM chat_group_msg WHERE group_id = $1 AND msg_id = $2 LIMIT 1",
				group_id,
				msg_id);
			if (rows.empty()) {
				txn.abort();
				return false;
			}
			const int from_uid = rows[0]["from_uid"].as<int>();
			if (from_uid != operator_uid && !HasGroupPermission(group_id, operator_uid, kPermDeleteMessages)) {
				txn.abort();
				return false;
			}

			const auto updated = txn.exec_params(
				"UPDATE chat_group_msg SET content = $1, msg_type = 'text', mentions_json = '[]', "
				"edited_at_ms = $2, deleted_at_ms = 0 WHERE group_id = $3 AND msg_id = $4",
				content,
				edited_at_ms,
				group_id,
				msg_id);
			if (updated.affected_rows() <= 0) {
				txn.abort();
				return false;
			}
			txn.exec_params(
				"DELETE FROM chat_group_msg_ext WHERE msg_id = $1",
				msg_id);
			txn.commit();
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
			return false;
		}
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> qmsg(
			con->_con->prepareStatement("SELECT from_uid FROM chat_group_msg WHERE group_id = ? AND msg_id = ? LIMIT 1"));
		qmsg->setInt64(1, group_id);
		qmsg->setString(2, msg_id);
		std::unique_ptr<sql::ResultSet> res(qmsg->executeQuery());
		if (!res->next()) {
			return false;
		}
		const int from_uid = res->getInt("from_uid");

		if (from_uid != operator_uid) {
			if (!HasGroupPermission(group_id, operator_uid, kPermDeleteMessages)) {
				return false;
			}
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> up(
			con->_con->prepareStatement("UPDATE chat_group_msg SET content = ?, msg_type = 'text', mentions_json = '[]', "
				"edited_at_ms = ?, deleted_at_ms = 0 WHERE group_id = ? AND msg_id = ?"));
		up->setString(1, content);
		up->setInt64(2, edited_at_ms);
		up->setInt64(3, group_id);
		up->setString(4, msg_id);
		if (up->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> del_ext(
			con->_con->prepareStatement("DELETE FROM chat_group_msg_ext WHERE msg_id = ?"));
		del_ext->setString(1, msg_id);
		del_ext->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::RevokeGroupMessage(const int64_t& group_id, const int& operator_uid, const std::string& msg_id, int64_t deleted_at_ms) {
	if (group_id <= 0 || operator_uid <= 0 || msg_id.empty()) {
		return false;
	}
	if (deleted_at_ms <= 0) {
		deleted_at_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			const auto rows = txn.exec_params(
				"SELECT from_uid FROM chat_group_msg WHERE group_id = $1 AND msg_id = $2 LIMIT 1",
				group_id,
				msg_id);
			if (rows.empty()) {
				txn.abort();
				return false;
			}
			const int from_uid = rows[0]["from_uid"].as<int>();
			if (from_uid != operator_uid && !HasGroupPermission(group_id, operator_uid, kPermDeleteMessages)) {
				txn.abort();
				return false;
			}

			const auto updated = txn.exec_params(
				"UPDATE chat_group_msg SET content = '[消息已撤回]', msg_type = 'revoke', mentions_json = '[]', "
				"deleted_at_ms = $1, edited_at_ms = 0 WHERE group_id = $2 AND msg_id = $3",
				deleted_at_ms,
				group_id,
				msg_id);
			if (updated.affected_rows() <= 0) {
				txn.abort();
				return false;
			}
			txn.exec_params(
				"DELETE FROM chat_group_msg_ext WHERE msg_id = $1",
				msg_id);
			txn.commit();
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
			return false;
		}
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> qmsg(
			con->_con->prepareStatement("SELECT from_uid FROM chat_group_msg WHERE group_id = ? AND msg_id = ? LIMIT 1"));
		qmsg->setInt64(1, group_id);
		qmsg->setString(2, msg_id);
		std::unique_ptr<sql::ResultSet> res(qmsg->executeQuery());
		if (!res->next()) {
			return false;
		}
		const int from_uid = res->getInt("from_uid");

		if (from_uid != operator_uid) {
			if (!HasGroupPermission(group_id, operator_uid, kPermDeleteMessages)) {
				return false;
			}
		}

		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> up(
			con->_con->prepareStatement("UPDATE chat_group_msg SET content = '[消息已撤回]', msg_type = 'revoke', mentions_json = '[]', "
				"deleted_at_ms = ?, edited_at_ms = 0 WHERE group_id = ? AND msg_id = ?"));
		up->setInt64(1, deleted_at_ms);
		up->setInt64(2, group_id);
		up->setString(3, msg_id);
		if (up->executeUpdate() <= 0) {
			con->_con->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> del_ext(
			con->_con->prepareStatement("DELETE FROM chat_group_msg_ext WHERE msg_id = ?"));
		del_ext->setString(1, msg_id);
		del_ext->executeUpdate();

		con->_con->commit();
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::GetGroupHistory(const int64_t& group_id, const int64_t& before_ts, const int64_t& before_seq, const int& limit,
	std::vector<std::shared_ptr<GroupMessageInfo>>& messages, bool& has_more) {
	has_more = false;
	messages.clear();

	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::read_transaction txn(conn);
			const int final_limit = std::max(1, std::min(limit, 50));
			pqxx::result rows;
			if (before_seq > 0) {
				rows = txn.exec_params(
					"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
					"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
					"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
					"FROM chat_group_msg m "
					"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
					"LEFT JOIN \"user\" u ON m.from_uid = u.uid "
					"WHERE m.group_id = $1 AND m.group_seq < $2 "
					"ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT $3",
					group_id,
					before_seq,
					final_limit + 1);
			}
			else if (before_ts > 0) {
				rows = txn.exec_params(
					"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
					"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
					"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
					"FROM chat_group_msg m "
					"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
					"LEFT JOIN \"user\" u ON m.from_uid = u.uid "
					"WHERE m.group_id = $1 AND m.created_at < $2 "
					"ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT $3",
					group_id,
					before_ts,
					final_limit + 1);
			}
			else {
				rows = txn.exec_params(
					"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
					"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
					"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
					"FROM chat_group_msg m "
					"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
					"LEFT JOIN \"user\" u ON m.from_uid = u.uid "
					"WHERE m.group_id = $1 ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT $2",
					group_id,
					final_limit + 1);
			}

			for (const auto& row : rows) {
				auto info = std::make_shared<GroupMessageInfo>();
				info->msg_id = row["msg_id"].c_str();
				info->group_id = row["group_id"].as<int64_t>();
				info->from_uid = row["from_uid"].as<int>();
				info->msg_type = row["msg_type"].is_null() ? "" : row["msg_type"].c_str();
				info->content = row["content"].is_null() ? "" : row["content"].c_str();
				info->mentions_json = row["mentions_json"].is_null() ? "" : row["mentions_json"].c_str();
				info->reply_to_server_msg_id = row["reply_to_server_msg_id"].is_null() ? 0 : row["reply_to_server_msg_id"].as<int64_t>();
				info->forward_meta_json = row["forward_meta_json"].is_null() ? "" : row["forward_meta_json"].c_str();
				info->edited_at_ms = row["edited_at_ms"].is_null() ? 0 : row["edited_at_ms"].as<int64_t>();
				info->deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
				info->created_at = row["created_at"].as<int64_t>();
				info->server_msg_id = row["server_msg_id"].as<int64_t>();
				info->group_seq = row["group_seq"].as<int64_t>();
				info->file_name = row["file_name"].is_null() ? "" : row["file_name"].c_str();
				info->mime = row["mime"].is_null() ? "" : row["mime"].c_str();
				info->size = row["size"].is_null() ? 0 : row["size"].as<int>();
				info->from_name = row["from_name"].is_null() ? "" : row["from_name"].c_str();
				info->from_nick = row["from_nick"].is_null() ? "" : row["from_nick"].c_str();
				info->from_icon = row["from_icon"].is_null() ? "" : row["from_icon"].c_str();
				messages.push_back(info);
			}
			if (static_cast<int>(messages.size()) > final_limit) {
				has_more = true;
				messages.resize(final_limit);
			}
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
			return false;
		}
	}

	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		const int final_limit = std::max(1, std::min(limit, 50));
		std::unique_ptr<sql::PreparedStatement> pstmt;
		if (before_seq > 0) {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"LEFT JOIN user u ON m.from_uid = u.uid "
				"WHERE m.group_id = ? AND m.group_seq < ? ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT ?"));
			pstmt->setInt64(1, group_id);
			pstmt->setInt64(2, before_seq);
			pstmt->setInt(3, final_limit + 1);
		}
		else if (before_ts > 0) {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"LEFT JOIN user u ON m.from_uid = u.uid "
				"WHERE m.group_id = ? AND m.created_at < ? ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT ?"));
			pstmt->setInt64(1, group_id);
			pstmt->setInt64(2, before_ts);
			pstmt->setInt(3, final_limit + 1);
		}
		else {
			pstmt.reset(con->_con->prepareStatement(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size, u.name AS from_name, u.nick AS from_nick, u.icon AS from_icon "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"LEFT JOIN user u ON m.from_uid = u.uid "
				"WHERE m.group_id = ? ORDER BY m.group_seq DESC, m.server_msg_id DESC LIMIT ?"));
			pstmt->setInt64(1, group_id);
			pstmt->setInt(2, final_limit + 1);
		}
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupMessageInfo>();
			info->msg_id = res->getString("msg_id");
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->from_uid = res->getInt("from_uid");
			info->msg_type = res->getString("msg_type");
			info->content = res->getString("content");
			info->mentions_json = res->getString("mentions_json");
			info->reply_to_server_msg_id = res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
			info->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
			info->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
			info->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
			info->created_at = static_cast<int64_t>(std::stoll(res->getString("created_at")));
			info->server_msg_id = static_cast<int64_t>(std::stoll(res->getString("server_msg_id")));
			info->group_seq = static_cast<int64_t>(std::stoll(res->getString("group_seq")));
			info->file_name = res->isNull("file_name") ? "" : res->getString("file_name");
			info->mime = res->isNull("mime") ? "" : res->getString("mime");
			info->size = res->isNull("size") ? 0 : res->getInt("size");
			info->from_name = res->isNull("from_name") ? "" : res->getString("from_name");
			info->from_nick = res->isNull("from_nick") ? "" : res->getString("from_nick");
			info->from_icon = res->isNull("from_icon") ? "" : res->getString("from_icon");
			messages.push_back(info);
		}
		if (static_cast<int>(messages.size()) > final_limit) {
			has_more = true;
			messages.resize(final_limit);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::GetGroupMessageByMsgId(const int64_t& group_id, const std::string& msg_id, std::shared_ptr<GroupMessageInfo>& message) {
	message = nullptr;
	if (group_id <= 0 || msg_id.empty()) {
		return false;
	}

	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::read_transaction txn(conn);
			const auto rows = txn.exec_params(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"WHERE m.group_id = $1 AND m.msg_id = $2 LIMIT 1",
				group_id,
				msg_id);
			if (rows.empty()) {
				return false;
			}

			const auto& row = rows[0];
			auto one = std::make_shared<GroupMessageInfo>();
			one->msg_id = row["msg_id"].c_str();
			one->group_id = row["group_id"].as<int64_t>();
			one->from_uid = row["from_uid"].as<int>();
			one->msg_type = row["msg_type"].is_null() ? "" : row["msg_type"].c_str();
			one->content = row["content"].is_null() ? "" : row["content"].c_str();
			one->mentions_json = row["mentions_json"].is_null() ? "[]" : row["mentions_json"].c_str();
			one->reply_to_server_msg_id = row["reply_to_server_msg_id"].is_null() ? 0 : row["reply_to_server_msg_id"].as<int64_t>();
			one->forward_meta_json = row["forward_meta_json"].is_null() ? "" : row["forward_meta_json"].c_str();
			one->edited_at_ms = row["edited_at_ms"].is_null() ? 0 : row["edited_at_ms"].as<int64_t>();
			one->deleted_at_ms = row["deleted_at_ms"].is_null() ? 0 : row["deleted_at_ms"].as<int64_t>();
			one->created_at = row["created_at"].as<int64_t>();
			one->server_msg_id = row["server_msg_id"].as<int64_t>();
			one->group_seq = row["group_seq"].as<int64_t>();
			one->file_name = row["file_name"].is_null() ? "" : row["file_name"].c_str();
			one->mime = row["mime"].is_null() ? "" : row["mime"].c_str();
			one->size = row["size"].is_null() ? 0 : row["size"].as<int>();
			message = one;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "PostgreSQL exception: " << e.what() << std::endl;
			return false;
		}
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
			con->_con->prepareStatement(
				"SELECT m.msg_id, m.server_msg_id, m.group_seq, m.group_id, m.from_uid, m.msg_type, m.content, m.mentions_json, "
				"m.reply_to_server_msg_id, m.forward_meta_json, m.edited_at_ms, m.deleted_at_ms, m.created_at, "
				"e.file_name, e.mime, e.size "
				"FROM chat_group_msg m "
				"LEFT JOIN chat_group_msg_ext e ON m.msg_id = e.msg_id "
				"WHERE m.group_id = ? AND m.msg_id = ? LIMIT 1"));
		pstmt->setInt64(1, group_id);
		pstmt->setString(2, msg_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}

		auto one = std::make_shared<GroupMessageInfo>();
		one->msg_id = res->getString("msg_id");
		one->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
		one->from_uid = res->getInt("from_uid");
		one->msg_type = res->getString("msg_type");
		one->content = res->getString("content");
		one->mentions_json = res->isNull("mentions_json") ? "[]" : res->getString("mentions_json");
		one->reply_to_server_msg_id = res->isNull("reply_to_server_msg_id") ? 0 : res->getInt64("reply_to_server_msg_id");
		one->forward_meta_json = res->isNull("forward_meta_json") ? "" : res->getString("forward_meta_json");
		one->edited_at_ms = res->isNull("edited_at_ms") ? 0 : res->getInt64("edited_at_ms");
		one->deleted_at_ms = res->isNull("deleted_at_ms") ? 0 : res->getInt64("deleted_at_ms");
		one->created_at = static_cast<int64_t>(std::stoll(res->getString("created_at")));
		one->server_msg_id = static_cast<int64_t>(std::stoll(res->getString("server_msg_id")));
		one->group_seq = static_cast<int64_t>(std::stoll(res->getString("group_seq")));
		one->file_name = res->isNull("file_name") ? "" : res->getString("file_name");
		one->mime = res->isNull("mime") ? "" : res->getString("mime");
		one->size = res->isNull("size") ? 0 : res->getInt("size");
		message = one;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
