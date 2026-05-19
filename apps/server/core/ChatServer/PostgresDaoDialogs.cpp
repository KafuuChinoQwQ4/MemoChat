#include "PostgresDao.h"
#include "ConfigMgr.h"
#include "db/PqxxCompat.h"
#include "PostgresPool.h"
#include "SnowflakeUtil.h"
#include <pqxx/pqxx>
#include <set>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <stdexcept>
#include <limits>
#include <sstream>

namespace {
bool IsValidGroupCode(const std::string& group_code) {
	if (group_code.size() != 10 || group_code[0] != 'g') {
		return false;
	}
	if (group_code[1] < '1' || group_code[1] > '9') {
		return false;
	}
	for (size_t i = 2; i < group_code.size(); ++i) {
		if (!std::isdigit(static_cast<unsigned char>(group_code[i]))) {
			return false;
		}
	}
	return true;
}

constexpr int64_t kPermChangeGroupInfo = 1LL << 0;
constexpr int64_t kPermDeleteMessages = 1LL << 1;
constexpr int64_t kPermInviteUsers = 1LL << 2;
constexpr int64_t kPermManageAdmins = 1LL << 3;
constexpr int64_t kPermPinMessages = 1LL << 4;
constexpr int64_t kPermBanUsers = 1LL << 5;
constexpr int64_t kPermManageTopics = 1LL << 6;
constexpr int64_t kDefaultAdminPermBits =
	kPermChangeGroupInfo | kPermDeleteMessages | kPermInviteUsers | kPermPinMessages | kPermBanUsers;
constexpr int64_t kOwnerPermBits =
	kDefaultAdminPermBits | kPermManageAdmins | kPermManageTopics;

std::string BuildPreviewText(const std::string& msg_type, const std::string& content) {
	if (msg_type == "image" || content.rfind("__memochat_img__:", 0) == 0) {
		return "[图片]";
	}
	if (msg_type == "file" || content.rfind("__memochat_file__:", 0) == 0) {
		return "[文件]";
	}
	if (msg_type == "call" || content.rfind("__memochat_call__:", 0) == 0) {
		return "[通话邀请]";
	}
	if (content.rfind("__memochat_reply__:", 0) == 0) {
		return "[回复]";
	}
	if (content.size() <= 80) {
		return content;
	}
	return content.substr(0, 80);
}

void ExecuteIgnoreSql(sql::Statement* stmt, const std::string& sql_text) {
	if (stmt == nullptr) {
		return;
	}
	try {
		stmt->execute(sql_text);
	}
	catch (const sql::SQLException&) {
	}
}

int64_t NowMsPostgresDao() {
	return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
}
}
bool PostgresDao::GetDialogMetaByOwner(const int& owner_uid, std::vector<std::shared_ptr<DialogMetaInfo>>& metas) {
	metas.clear();
	if (owner_uid <= 0) {
		return false;
	}
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::read_transaction txn(conn);
			const auto rows = txn.exec_params(
				"SELECT dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state "
				"FROM chat_dialog WHERE owner_uid = $1",
				owner_uid);
			for (const auto& row : rows) {
				auto info = std::make_shared<DialogMetaInfo>();
				info->owner_uid = owner_uid;
				info->dialog_type = row["dialog_type"].is_null() ? "" : row["dialog_type"].c_str();
				info->peer_uid = row["peer_uid"].is_null() ? 0 : row["peer_uid"].as<int>();
				info->group_id = row["group_id"].is_null() ? 0 : row["group_id"].as<int64_t>();
				info->pinned_rank = row["pinned_rank"].is_null() ? 0 : row["pinned_rank"].as<int>();
				info->draft_text = row["draft_text"].is_null() ? "" : row["draft_text"].c_str();
				info->mute_state = row["mute_state"].is_null() ? 0 : row["mute_state"].as<int>();
				metas.push_back(info);
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state "
				"FROM chat_dialog WHERE owner_uid = ?"));
		pstmt->setInt(1, owner_uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<DialogMetaInfo>();
			info->owner_uid = owner_uid;
			info->dialog_type = res->getString("dialog_type");
			info->peer_uid = res->getInt("peer_uid");
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->pinned_rank = res->getInt("pinned_rank");
			info->draft_text = res->isNull("draft_text") ? "" : res->getString("draft_text");
			info->mute_state = res->getInt("mute_state");
			metas.push_back(info);
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

bool PostgresDao::GetPrivateDialogRuntime(const int& owner_uid, const int& peer_uid, DialogRuntimeInfo& runtime) {
	runtime = DialogRuntimeInfo();
	if (owner_uid <= 0 || peer_uid <= 0) {
		return false;
	}
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::read_transaction txn(conn);
			const auto rows = txn.exec_params(
				"SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
				"WHERE owner_uid = $1 AND dialog_type = 'private' AND peer_uid = $2 AND group_id = 0 LIMIT 1",
				owner_uid,
				peer_uid);
			if (!rows.empty()) {
				const auto& row = rows[0];
				runtime.last_msg_preview = row["last_msg_preview"].is_null() ? "" : row["last_msg_preview"].c_str();
				runtime.last_msg_ts = row["last_msg_ts"].is_null() ? 0 : row["last_msg_ts"].as<int64_t>();
				runtime.unread_count = row["unread_count"].is_null() ? 0 : std::max(0, row["unread_count"].as<int>());
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
				"WHERE owner_uid = ? AND dialog_type = 'private' AND peer_uid = ? AND group_id = 0 LIMIT 1"));
		pstmt->setInt(1, owner_uid);
		pstmt->setInt(2, peer_uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (res->next()) {
			runtime.last_msg_preview = res->isNull("last_msg_preview") ? "" : res->getString("last_msg_preview");
			runtime.last_msg_ts = res->isNull("last_msg_ts") ? 0 : res->getInt64("last_msg_ts");
			runtime.unread_count = std::max(0, res->getInt("unread_count"));
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

bool PostgresDao::GetGroupDialogRuntime(const int& owner_uid, const int64_t& group_id, DialogRuntimeInfo& runtime) {
	runtime = DialogRuntimeInfo();
	if (owner_uid <= 0 || group_id <= 0) {
		return false;
	}
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::read_transaction txn(conn);
			const auto rows = txn.exec_params(
				"SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
				"WHERE owner_uid = $1 AND dialog_type = 'group' AND group_id = $2 LIMIT 1",
				owner_uid,
				group_id);
			if (!rows.empty()) {
				const auto& row = rows[0];
				runtime.last_msg_preview = row["last_msg_preview"].is_null() ? "" : row["last_msg_preview"].c_str();
				runtime.last_msg_ts = row["last_msg_ts"].is_null() ? 0 : row["last_msg_ts"].as<int64_t>();
				runtime.unread_count = row["unread_count"].is_null() ? 0 : std::max(0, row["unread_count"].as<int>());
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT last_msg_preview, last_msg_ts, unread_count FROM chat_dialog "
				"WHERE owner_uid = ? AND dialog_type = 'group' AND group_id = ? LIMIT 1"));
		pstmt->setInt(1, owner_uid);
		pstmt->setInt64(2, group_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (res->next()) {
			runtime.last_msg_preview = res->isNull("last_msg_preview") ? "" : res->getString("last_msg_preview");
			runtime.last_msg_ts = res->isNull("last_msg_ts") ? 0 : res->getInt64("last_msg_ts");
			runtime.unread_count = std::max(0, res->getInt("unread_count"));
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

bool PostgresDao::RefreshDialogsForOwner(const int& owner_uid) {
	if (owner_uid <= 0) {
		return false;
	}

	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			std::vector<int> peers;
			{
				const auto peer_rows = txn.exec_params("SELECT friend_id FROM friend WHERE self_id = $1", owner_uid);
				for (const auto& row : peer_rows) {
					const auto peer_uid = row["friend_id"].as<int>();
					if (peer_uid > 0) {
						peers.push_back(peer_uid);
					}
				}
			}

			for (int peer_uid : peers) {
				txn.exec_params0(
					"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
					"VALUES($1, 'private', $2, 0, 0, '', 0, '', 0, 0) "
					"ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO NOTHING",
					owner_uid,
					peer_uid);

				const int conv_min = std::min(owner_uid, peer_uid);
				const int conv_max = std::max(owner_uid, peer_uid);
				std::string preview;
				int64_t last_msg_ts = 0;
				int unread_count = 0;

				const auto last_rows = txn.exec_params(
					"SELECT content, created_at FROM chat_private_msg "
					"WHERE conv_uid_min = $1 AND conv_uid_max = $2 "
					"ORDER BY created_at DESC, msg_id DESC LIMIT 1",
					conv_min,
					conv_max);
				if (!last_rows.empty()) {
					const auto content = last_rows[0]["content"].is_null() ? "" : std::string(last_rows[0]["content"].c_str());
					preview = BuildPreviewText("", content);
					last_msg_ts = last_rows[0]["created_at"].is_null() ? 0 : last_rows[0]["created_at"].as<int64_t>();
				}

				const auto unread_rows = txn.exec_params(
					"SELECT COUNT(1) AS unread_count FROM chat_private_msg "
					"WHERE conv_uid_min = $1 AND conv_uid_max = $2 AND from_uid = $3 "
					"AND created_at > COALESCE((SELECT read_ts FROM chat_private_read_state WHERE uid = $4 AND peer_uid = $5), 0)",
					conv_min,
					conv_max,
					peer_uid,
					owner_uid,
					peer_uid);
				if (!unread_rows.empty()) {
					unread_count = std::max(0, unread_rows[0]["unread_count"].as<int>());
				}

				txn.exec_params0(
					"UPDATE chat_dialog SET last_msg_preview = $1, last_msg_ts = $2, unread_count = $3, updated_at = CURRENT_TIMESTAMP "
					"WHERE owner_uid = $4 AND dialog_type = 'private' AND peer_uid = $5 AND group_id = 0",
					preview,
					last_msg_ts,
					unread_count,
					owner_uid,
					peer_uid);
			}

			std::vector<int64_t> groups;
			{
				const auto group_rows = txn.exec_params(
					"SELECT m.group_id FROM chat_group_member m "
					"JOIN chat_group g ON g.group_id = m.group_id "
					"WHERE m.uid = $1 AND m.status = 1 AND g.status = 1",
					owner_uid);
				for (const auto& row : group_rows) {
					const auto group_id = row["group_id"].as<int64_t>();
					if (group_id > 0) {
						groups.push_back(group_id);
					}
				}
			}

			for (int64_t group_id : groups) {
				txn.exec_params0(
					"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
					"VALUES($1, 'group', 0, $2, 0, '', 0, '', 0, 0) "
					"ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO NOTHING",
					owner_uid,
					group_id);

				std::string preview;
				int64_t last_msg_ts = 0;
				int unread_count = 0;

				const auto last_rows = txn.exec_params(
					"SELECT msg_type, content, created_at FROM chat_group_msg "
					"WHERE group_id = $1 ORDER BY group_seq DESC, server_msg_id DESC LIMIT 1",
					group_id);
				if (!last_rows.empty()) {
					const auto msg_type = last_rows[0]["msg_type"].is_null() ? "" : std::string(last_rows[0]["msg_type"].c_str());
					const auto content = last_rows[0]["content"].is_null() ? "" : std::string(last_rows[0]["content"].c_str());
					preview = BuildPreviewText(msg_type, content);
					last_msg_ts = last_rows[0]["created_at"].is_null() ? 0 : last_rows[0]["created_at"].as<int64_t>();
				}

				const auto unread_rows = txn.exec_params(
					"SELECT COUNT(1) AS unread_count FROM chat_group_msg "
					"WHERE group_id = $1 AND created_at > COALESCE((SELECT read_ts FROM chat_group_read_state WHERE uid = $2 AND group_id = $3), 0) "
					"AND from_uid <> $4",
					group_id,
					owner_uid,
					group_id,
					owner_uid);
				if (!unread_rows.empty()) {
					unread_count = std::max(0, unread_rows[0]["unread_count"].as<int>());
				}

				txn.exec_params0(
					"UPDATE chat_dialog SET last_msg_preview = $1, last_msg_ts = $2, unread_count = $3, updated_at = CURRENT_TIMESTAMP "
					"WHERE owner_uid = $4 AND dialog_type = 'group' AND group_id = $5",
					preview,
					last_msg_ts,
					unread_count,
					owner_uid,
					group_id);
			}

			txn.exec_params0(
				"DELETE FROM chat_dialog "
				"WHERE owner_uid = $1 AND dialog_type = 'private' AND peer_uid > 0 "
				"AND NOT EXISTS (SELECT 1 FROM friend f WHERE f.self_id = $2 AND f.friend_id = chat_dialog.peer_uid)",
				owner_uid,
				owner_uid);
			txn.exec_params0(
				"DELETE FROM chat_dialog "
				"WHERE owner_uid = $1 AND dialog_type = 'group' AND group_id > 0 "
				"AND NOT EXISTS ("
				"SELECT 1 FROM chat_group_member m JOIN chat_group g ON g.group_id = m.group_id "
				"WHERE m.uid = $2 AND m.group_id = chat_dialog.group_id AND m.status = 1 AND g.status = 1)",
				owner_uid,
				owner_uid);

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
		std::vector<int> peers;
		{
			std::unique_ptr<sql::PreparedStatement> list_private(
				con->_con->prepareStatement("SELECT friend_id FROM friend WHERE self_id = ?"));
			list_private->setInt(1, owner_uid);
			std::unique_ptr<sql::ResultSet> private_res(list_private->executeQuery());
			while (private_res->next()) {
				const int peer_uid = private_res->getInt("friend_id");
				if (peer_uid > 0) {
					peers.push_back(peer_uid);
				}
			}
		}

		for (int peer_uid : peers) {
			std::unique_ptr<sql::PreparedStatement> ensure_row(
				con->_con->prepareStatement(
					"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
					"VALUES(?, 'private', ?, 0, 0, '', 0, '', 0, 0) "
					"ON DUPLICATE KEY UPDATE owner_uid = owner_uid"));
			ensure_row->setInt(1, owner_uid);
			ensure_row->setInt(2, peer_uid);
			ensure_row->executeUpdate();

			const int conv_min = std::min(owner_uid, peer_uid);
			const int conv_max = std::max(owner_uid, peer_uid);
			std::string preview;
			int64_t last_msg_ts = 0;
			int unread_count = 0;

			{
				std::unique_ptr<sql::PreparedStatement> last_stmt(
					con->_con->prepareStatement(
						"SELECT content, created_at FROM chat_private_msg "
						"WHERE conv_uid_min = ? AND conv_uid_max = ? "
						"ORDER BY created_at DESC, msg_id DESC LIMIT 1"));
				last_stmt->setInt(1, conv_min);
				last_stmt->setInt(2, conv_max);
				std::unique_ptr<sql::ResultSet> last_res(last_stmt->executeQuery());
				if (last_res->next()) {
					const std::string content = last_res->isNull("content") ? "" : last_res->getString("content");
					preview = BuildPreviewText("", content);
					last_msg_ts = last_res->isNull("created_at") ? 0 : last_res->getInt64("created_at");
				}
			}

			{
				std::unique_ptr<sql::PreparedStatement> unread_stmt(
					con->_con->prepareStatement(
						"SELECT COUNT(1) AS unread_count FROM chat_private_msg "
						"WHERE conv_uid_min = ? AND conv_uid_max = ? AND from_uid = ? "
						"AND created_at > COALESCE((SELECT read_ts FROM chat_private_read_state WHERE uid = ? AND peer_uid = ?), 0)"));
				unread_stmt->setInt(1, conv_min);
				unread_stmt->setInt(2, conv_max);
				unread_stmt->setInt(3, peer_uid);
				unread_stmt->setInt(4, owner_uid);
				unread_stmt->setInt(5, peer_uid);
				std::unique_ptr<sql::ResultSet> unread_res(unread_stmt->executeQuery());
				if (unread_res->next()) {
					unread_count = std::max(0, unread_res->getInt("unread_count"));
				}
			}

			std::unique_ptr<sql::PreparedStatement> update_stmt(
				con->_con->prepareStatement(
					"UPDATE chat_dialog SET last_msg_preview = ?, last_msg_ts = ?, unread_count = ?, updated_at = CURRENT_TIMESTAMP "
					"WHERE owner_uid = ? AND dialog_type = 'private' AND peer_uid = ? AND group_id = 0"));
			update_stmt->setString(1, preview);
			update_stmt->setInt64(2, last_msg_ts);
			update_stmt->setInt(3, unread_count);
			update_stmt->setInt(4, owner_uid);
			update_stmt->setInt(5, peer_uid);
			update_stmt->executeUpdate();
		}

		std::vector<int64_t> groups;
		{
			std::unique_ptr<sql::PreparedStatement> list_groups(
				con->_con->prepareStatement(
					"SELECT m.group_id FROM chat_group_member m "
					"JOIN chat_group g ON g.group_id = m.group_id "
					"WHERE m.uid = ? AND m.status = 1 AND g.status = 1"));
			list_groups->setInt(1, owner_uid);
			std::unique_ptr<sql::ResultSet> group_res(list_groups->executeQuery());
			while (group_res->next()) {
				const int64_t group_id = group_res->getInt64("group_id");
				if (group_id > 0) {
					groups.push_back(group_id);
				}
			}
		}

		for (int64_t group_id : groups) {
			std::unique_ptr<sql::PreparedStatement> ensure_row(
				con->_con->prepareStatement(
					"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
					"VALUES(?, 'group', 0, ?, 0, '', 0, '', 0, 0) "
					"ON DUPLICATE KEY UPDATE owner_uid = owner_uid"));
			ensure_row->setInt(1, owner_uid);
			ensure_row->setInt64(2, group_id);
			ensure_row->executeUpdate();

			std::string preview;
			int64_t last_msg_ts = 0;
			int unread_count = 0;

			{
				std::unique_ptr<sql::PreparedStatement> last_stmt(
					con->_con->prepareStatement(
						"SELECT msg_type, content, created_at FROM chat_group_msg "
						"WHERE group_id = ? ORDER BY group_seq DESC, server_msg_id DESC LIMIT 1"));
				last_stmt->setInt64(1, group_id);
				std::unique_ptr<sql::ResultSet> last_res(last_stmt->executeQuery());
				if (last_res->next()) {
					const std::string msg_type = last_res->isNull("msg_type") ? "" : last_res->getString("msg_type");
					const std::string content = last_res->isNull("content") ? "" : last_res->getString("content");
					preview = BuildPreviewText(msg_type, content);
					last_msg_ts = last_res->isNull("created_at") ? 0 : last_res->getInt64("created_at");
				}
			}

			{
				std::unique_ptr<sql::PreparedStatement> unread_stmt(
					con->_con->prepareStatement(
						"SELECT COUNT(1) AS unread_count FROM chat_group_msg "
						"WHERE group_id = ? AND created_at > COALESCE((SELECT read_ts FROM chat_group_read_state WHERE uid = ? AND group_id = ?), 0) "
						"AND from_uid <> ?"));
				unread_stmt->setInt64(1, group_id);
				unread_stmt->setInt(2, owner_uid);
				unread_stmt->setInt64(3, group_id);
				unread_stmt->setInt(4, owner_uid);
				std::unique_ptr<sql::ResultSet> unread_res(unread_stmt->executeQuery());
				if (unread_res->next()) {
					unread_count = std::max(0, unread_res->getInt("unread_count"));
				}
			}

			std::unique_ptr<sql::PreparedStatement> update_stmt(
				con->_con->prepareStatement(
					"UPDATE chat_dialog SET last_msg_preview = ?, last_msg_ts = ?, unread_count = ?, updated_at = CURRENT_TIMESTAMP "
					"WHERE owner_uid = ? AND dialog_type = 'group' AND group_id = ?"));
			update_stmt->setString(1, preview);
			update_stmt->setInt64(2, last_msg_ts);
			update_stmt->setInt(3, unread_count);
			update_stmt->setInt(4, owner_uid);
			update_stmt->setInt64(5, group_id);
			update_stmt->executeUpdate();
		}

		{
			std::unique_ptr<sql::PreparedStatement> clean_private(
				con->_con->prepareStatement(
					"DELETE FROM chat_dialog "
					"WHERE owner_uid = ? AND dialog_type = 'private' AND peer_uid > 0 "
					"AND NOT EXISTS (SELECT 1 FROM friend f WHERE f.self_id = ? AND f.friend_id = chat_dialog.peer_uid)"));
			clean_private->setInt(1, owner_uid);
			clean_private->setInt(2, owner_uid);
			clean_private->executeUpdate();
		}
		{
			std::unique_ptr<sql::PreparedStatement> clean_group(
				con->_con->prepareStatement(
					"DELETE FROM chat_dialog "
					"WHERE owner_uid = ? AND dialog_type = 'group' AND group_id > 0 "
					"AND NOT EXISTS ("
					"SELECT 1 FROM chat_group_member m JOIN chat_group g ON g.group_id = m.group_id "
					"WHERE m.uid = ? AND m.group_id = chat_dialog.group_id AND m.status = 1 AND g.status = 1)"));
			clean_group->setInt(1, owner_uid);
			clean_group->setInt(2, owner_uid);
			clean_group->executeUpdate();
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

bool PostgresDao::UpsertGroupReadState(const int& uid, const int64_t& group_id, const int64_t& read_ts) {
	if (uid <= 0 || group_id <= 0) {
		return false;
	}
	int64_t normalized_read_ts = read_ts;
	if (normalized_read_ts <= 0) {
		normalized_read_ts = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}

	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			const auto result = txn.exec_params(
				"INSERT INTO chat_group_read_state(uid, group_id, read_ts) VALUES($1, $2, $3) "
				"ON CONFLICT (uid, group_id) DO UPDATE SET "
				"read_ts = GREATEST(chat_group_read_state.read_ts, EXCLUDED.read_ts), "
				"updated_at = CURRENT_TIMESTAMP",
				uid,
				group_id,
				normalized_read_ts);
			txn.commit();
			return result.affected_rows() >= 0;
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
				"INSERT INTO chat_group_read_state(uid, group_id, read_ts) VALUES(?, ?, ?) "
				"ON DUPLICATE KEY UPDATE read_ts = GREATEST(read_ts, VALUES(read_ts)), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, uid);
		pstmt->setInt64(2, group_id);
		pstmt->setInt64(3, normalized_read_ts);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::UpsertDialogDraft(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const std::string& draft_text) {
	if (owner_uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		return false;
	}

	const int normalized_peer_uid = (dialog_type == "private") ? peer_uid : 0;
	const int64_t normalized_group_id = (dialog_type == "group") ? group_id : 0;
	if ((dialog_type == "private" && normalized_peer_uid <= 0)
		|| (dialog_type == "group" && normalized_group_id <= 0)) {
		return false;
	}

	std::string normalized_draft = draft_text;
	if (normalized_draft.size() > 2000) {
		normalized_draft.resize(2000);
	}
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			txn.exec_params(
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES($1, $2, $3, $4, 0, $5, 0, '', 0, 0) "
				"ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO UPDATE SET "
				"draft_text = EXCLUDED.draft_text, updated_at = CURRENT_TIMESTAMP",
				owner_uid,
				dialog_type,
				normalized_peer_uid,
				normalized_group_id,
				normalized_draft);
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES(?, ?, ?, ?, 0, ?, 0, '', 0, 0) "
				"ON DUPLICATE KEY UPDATE draft_text = VALUES(draft_text), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, owner_uid);
		pstmt->setString(2, dialog_type);
		pstmt->setInt(3, normalized_peer_uid);
		pstmt->setInt64(4, normalized_group_id);
		pstmt->setString(5, normalized_draft);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::UpsertDialogPinned(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const int& pinned_rank) {
	if (owner_uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		return false;
	}
	const int normalized_peer_uid = (dialog_type == "private") ? peer_uid : 0;
	const int64_t normalized_group_id = (dialog_type == "group") ? group_id : 0;
	if ((dialog_type == "private" && normalized_peer_uid <= 0)
		|| (dialog_type == "group" && normalized_group_id <= 0)) {
		return false;
	}
	const int normalized_pinned_rank = std::max(0, pinned_rank);
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			txn.exec_params(
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES($1, $2, $3, $4, $5, '', 0, '', 0, 0) "
				"ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO UPDATE SET "
				"pinned_rank = EXCLUDED.pinned_rank, updated_at = CURRENT_TIMESTAMP",
				owner_uid,
				dialog_type,
				normalized_peer_uid,
				normalized_group_id,
				normalized_pinned_rank);
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES(?, ?, ?, ?, ?, '', 0, '', 0, 0) "
				"ON DUPLICATE KEY UPDATE pinned_rank = VALUES(pinned_rank), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, owner_uid);
		pstmt->setString(2, dialog_type);
		pstmt->setInt(3, normalized_peer_uid);
		pstmt->setInt64(4, normalized_group_id);
		pstmt->setInt(5, normalized_pinned_rank);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::UpsertDialogMuteState(const int& owner_uid, const std::string& dialog_type, const int& peer_uid,
	const int64_t& group_id, const int& mute_state) {
	if (owner_uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		return false;
	}
	const int normalized_peer_uid = (dialog_type == "private") ? peer_uid : 0;
	const int64_t normalized_group_id = (dialog_type == "group") ? group_id : 0;
	if ((dialog_type == "private" && normalized_peer_uid <= 0)
		|| (dialog_type == "group" && normalized_group_id <= 0)) {
		return false;
	}
	const int normalized_mute_state = mute_state > 0 ? 1 : 0;
	if (use_postgres_) {
		try {
			pqxx::connection conn(postgres_connection_string_);
			pqxx::work txn(conn);
			txn.exec_params(
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES($1, $2, $3, $4, 0, '', $5, '', 0, 0) "
				"ON CONFLICT (owner_uid, dialog_type, peer_uid, group_id) DO UPDATE SET "
				"mute_state = EXCLUDED.mute_state, updated_at = CURRENT_TIMESTAMP",
				owner_uid,
				dialog_type,
				normalized_peer_uid,
				normalized_group_id,
				normalized_mute_state);
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
				"VALUES(?, ?, ?, ?, 0, '', ?, '', 0, 0) "
				"ON DUPLICATE KEY UPDATE mute_state = VALUES(mute_state), updated_at = CURRENT_TIMESTAMP"));
		pstmt->setInt(1, owner_uid);
		pstmt->setString(2, dialog_type);
		pstmt->setInt(3, normalized_peer_uid);
		pstmt->setInt64(4, normalized_group_id);
		pstmt->setInt(5, normalized_mute_state);
		return pstmt->executeUpdate() >= 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

std::string PostgresDao::GenerateGroupCode() {
	return SnowflakeUtil::formatPublicId(SnowflakeUtil::getInstance().nextId(), 'g');
}

bool PostgresDao::EnsureChatMessageIdempotencySchema() {
	if (!use_postgres_) {
		return false;
	}
	try {
		pqxx::connection conn(postgres_connection_string_);
		pqxx::work txn(conn);
		txn.exec0("CREATE UNIQUE INDEX IF NOT EXISTS uk_chat_private_msg_msg_id ON chat_private_msg(msg_id)");
		txn.exec0("CREATE UNIQUE INDEX IF NOT EXISTS uk_chat_group_msg_group_msg_id ON chat_group_msg(group_id, msg_id)");
		txn.commit();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "EnsureChatMessageIdempotencySchema PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::EnsureChatEventOutboxSchema() {
	if (!use_postgres_) {
		return false;
	}
	try {
		pqxx::connection conn(postgres_connection_string_);
		pqxx::work txn(conn);
		txn.exec0(
			"CREATE TABLE IF NOT EXISTS chat_event_outbox ("
			"id BIGSERIAL PRIMARY KEY,"
			"event_id VARCHAR(64) NOT NULL,"
			"topic VARCHAR(128) NOT NULL,"
			"partition_key VARCHAR(128) NOT NULL,"
			"payload_json JSONB NOT NULL,"
			"status SMALLINT NOT NULL DEFAULT 0,"
			"retry_count INT NOT NULL DEFAULT 0,"
			"next_retry_at BIGINT NOT NULL DEFAULT 0,"
			"created_at BIGINT NOT NULL,"
			"published_at BIGINT NULL,"
			"last_error TEXT NOT NULL DEFAULT '',"
			"CONSTRAINT uq_chat_event_outbox_event_id UNIQUE(event_id)"
			")");
		txn.exec0("CREATE INDEX IF NOT EXISTS idx_chat_event_outbox_status_retry ON chat_event_outbox(status, next_retry_at, id)");
		txn.exec0("CREATE INDEX IF NOT EXISTS idx_chat_event_outbox_topic_status ON chat_event_outbox(topic, status, id)");
		txn.commit();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "EnsureChatEventOutboxSchema PostgreSQL exception: " << e.what() << std::endl;
		return false;
	}
}

bool PostgresDao::EnsureDialogMetaSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_dialog ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"owner_uid INT NOT NULL,"
			"dialog_type VARCHAR(16) NOT NULL,"
			"peer_uid INT NOT NULL DEFAULT 0,"
			"group_id BIGINT NOT NULL DEFAULT 0,"
			"pinned_rank INT NOT NULL DEFAULT 0,"
			"draft_text VARCHAR(2000) NOT NULL DEFAULT '',"
			"mute_state TINYINT NOT NULL DEFAULT 0,"
			"last_msg_preview VARCHAR(255) NOT NULL DEFAULT '',"
			"last_msg_ts BIGINT NOT NULL DEFAULT 0,"
			"unread_count INT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_owner_dialog(owner_uid, dialog_type, peer_uid, group_id),"
			"KEY idx_owner(owner_uid),"
			"KEY idx_owner_type(owner_uid, dialog_type)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN mute_state TINYINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN last_msg_preview VARCHAR(255) NOT NULL DEFAULT ''");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN last_msg_ts BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog ADD COLUMN unread_count INT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog MODIFY COLUMN draft_text VARCHAR(2000) NOT NULL DEFAULT ''");
		}
		catch (sql::SQLException&) {
		}
		ExecuteIgnoreSql(stmt.get(), "CREATE UNIQUE INDEX uq_owner_dialog ON chat_dialog(owner_uid, dialog_type, peer_uid, group_id)");
		ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_owner_type ON chat_dialog(owner_uid, dialog_type)");
		ExecuteIgnoreSql(stmt.get(), "DROP INDEX uq_chat_dialog_owner_dialog ON chat_dialog");
		ExecuteIgnoreSql(stmt.get(), "DROP INDEX idx_chat_dialog_owner_type ON chat_dialog");

		// Keep old table for migration compatibility with historical deployments.
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_dialog_meta ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"owner_uid INT NOT NULL,"
			"dialog_type VARCHAR(16) NOT NULL,"
			"peer_uid INT NOT NULL DEFAULT 0,"
			"group_id BIGINT NOT NULL DEFAULT 0,"
			"pinned_rank INT NOT NULL DEFAULT 0,"
			"draft_text VARCHAR(2000) NOT NULL DEFAULT '',"
			"mute_state TINYINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_owner_dialog(owner_uid, dialog_type, peer_uid, group_id),"
			"KEY idx_owner(owner_uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_dialog_meta ADD COLUMN mute_state TINYINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_dialog_meta MODIFY COLUMN draft_text VARCHAR(2000) NOT NULL DEFAULT ''");
		}
		catch (sql::SQLException&) {
		}
		stmt->execute("INSERT INTO chat_dialog(owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, last_msg_preview, last_msg_ts, unread_count) "
			"SELECT owner_uid, dialog_type, peer_uid, group_id, pinned_rank, draft_text, mute_state, '', 0, 0 "
			"FROM chat_dialog_meta "
			"ON DUPLICATE KEY UPDATE "
			"pinned_rank = CASE WHEN chat_dialog.pinned_rank = 0 THEN VALUES(pinned_rank) ELSE chat_dialog.pinned_rank END, "
			"draft_text = CASE WHEN chat_dialog.draft_text = '' THEN VALUES(draft_text) ELSE chat_dialog.draft_text END, "
			"mute_state = CASE WHEN chat_dialog.mute_state = 0 THEN VALUES(mute_state) ELSE chat_dialog.mute_state END");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureDialogMetaSchema SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::EnsurePrivateReadStateSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_private_read_state ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"uid INT NOT NULL,"
			"peer_uid INT NOT NULL,"
			"read_ts BIGINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_uid_peer(uid, peer_uid),"
			"KEY idx_peer_uid(peer_uid, uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_private_read_state ADD COLUMN read_ts BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsurePrivateReadStateSchema SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::EnsureGroupReadStateSchema() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_read_state ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"uid INT NOT NULL,"
			"group_id BIGINT NOT NULL,"
			"read_ts BIGINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_uid_group(uid, group_id),"
			"KEY idx_group_uid(group_id, uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_group_read_state ADD COLUMN read_ts BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureGroupReadStateSchema SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::EnsureGroupMessageOrderSchema() {
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
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN server_msg_id BIGINT NOT NULL AUTO_INCREMENT UNIQUE");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN group_seq BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN reply_to_server_msg_id BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN forward_meta_json TEXT");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN edited_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group_msg ADD COLUMN deleted_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN reply_to_server_msg_id BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN forward_meta_json TEXT");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN edited_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_private_msg ADD COLUMN deleted_at_ms BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}
		ExecuteIgnoreSql(stmt.get(), "CREATE INDEX idx_group_seq ON chat_group_msg(group_id, group_seq)");

		stmt->execute("CREATE TEMPORARY TABLE IF NOT EXISTS tmp_group_seq_backfill ("
			"msg_id VARCHAR(64) PRIMARY KEY,"
			"group_seq BIGINT NOT NULL"
			") ENGINE=InnoDB");
		stmt->execute("TRUNCATE TABLE tmp_group_seq_backfill");
		stmt->execute("SET @memochat_prev_gid := -1, @memochat_seq := 0");
		stmt->execute(
			"INSERT INTO tmp_group_seq_backfill(msg_id, group_seq) "
			"SELECT ranked.msg_id, ranked.group_seq "
			"FROM ("
			"SELECT ordered.msg_id, "
			"(@memochat_seq := IF(@memochat_prev_gid = ordered.group_id, @memochat_seq + 1, 1)) AS group_seq, "
			"(@memochat_prev_gid := ordered.group_id) AS prev_gid "
			"FROM (SELECT msg_id, group_id FROM chat_group_msg ORDER BY group_id ASC, created_at ASC, msg_id ASC) ordered"
			") ranked");
		stmt->execute(
			"UPDATE chat_group_msg m "
			"JOIN tmp_group_seq_backfill t ON t.msg_id = m.msg_id "
			"SET m.group_seq = t.group_seq");
		stmt->execute("DROP TEMPORARY TABLE IF EXISTS tmp_group_seq_backfill");

		ExecuteIgnoreSql(stmt.get(), "DROP INDEX idx_chat_group_msg_group_seq ON chat_group_msg");
		ExecuteIgnoreSql(stmt.get(), "CREATE UNIQUE INDEX uk_chat_group_msg_group_seq ON chat_group_msg(group_id, group_seq)");
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "EnsureGroupMessageOrderSchema SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::EnsureGroupPermissionSchemaAndBackfill() {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		stmt->execute("CREATE TABLE IF NOT EXISTS chat_group_admin_permission ("
			"id BIGINT AUTO_INCREMENT PRIMARY KEY,"
			"group_id BIGINT NOT NULL,"
			"uid INT NOT NULL,"
			"permission_bits BIGINT NOT NULL DEFAULT 0,"
			"updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
			"UNIQUE KEY uq_group_uid(group_id, uid),"
			"KEY idx_uid(uid)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
		try {
			stmt->execute("ALTER TABLE chat_group_admin_permission ADD COLUMN permission_bits BIGINT NOT NULL DEFAULT 0");
		}
		catch (sql::SQLException&) {
		}

		con->_con->setAutoCommit(false);
		stmt->execute("DELETE p FROM chat_group_admin_permission p "
			"LEFT JOIN chat_group_member m ON p.group_id = m.group_id AND p.uid = m.uid "
			"WHERE m.id IS NULL OR m.status <> 1 OR m.role < 2");

		std::unique_ptr<sql::PreparedStatement> upsert_owner(
			con->_con->prepareStatement(
				"INSERT INTO chat_group_admin_permission(group_id, uid, permission_bits) "
				"SELECT group_id, uid, ? FROM chat_group_member WHERE status = 1 AND role = 3 "
				"ON DUPLICATE KEY UPDATE permission_bits = VALUES(permission_bits), updated_at = CURRENT_TIMESTAMP"));
		upsert_owner->setInt64(1, kOwnerPermBits);
		upsert_owner->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> upsert_admin(
			con->_con->prepareStatement(
				"INSERT INTO chat_group_admin_permission(group_id, uid, permission_bits) "
				"SELECT group_id, uid, ? FROM chat_group_member WHERE status = 1 AND role = 2 "
				"ON DUPLICATE KEY UPDATE permission_bits = IF(permission_bits <= 0, VALUES(permission_bits), permission_bits), updated_at = CURRENT_TIMESTAMP"));
		upsert_admin->setInt64(1, kDefaultAdminPermBits);
		upsert_admin->executeUpdate();

		con->_con->commit();
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
		std::cerr << "EnsureGroupPermissionSchemaAndBackfill SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::EnsureGroupCodeSchemaAndBackfill() {
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
			stmt->execute("ALTER TABLE chat_group ADD COLUMN group_code VARCHAR(10) NULL");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("ALTER TABLE chat_group ADD COLUMN icon VARCHAR(512) NULL");
		}
		catch (sql::SQLException&) {
		}
		try {
			stmt->execute("CREATE UNIQUE INDEX uk_chat_group_code ON chat_group(group_code)");
		}
		catch (sql::SQLException&) {
		}

		std::unique_ptr<sql::PreparedStatement> query(
			con->_con->prepareStatement("SELECT group_id, group_code FROM chat_group ORDER BY group_id ASC"));
		std::unique_ptr<sql::ResultSet> rows(query->executeQuery());

		struct GroupRow {
			int64_t group_id = 0;
			std::string group_code;
		};
		std::vector<GroupRow> all_rows;
		std::unordered_set<std::string> used_codes;
		int duplicate_fixed = 0;

		while (rows->next()) {
			GroupRow row;
			row.group_id = static_cast<int64_t>(std::stoll(rows->getString("group_id")));
			row.group_code = rows->isNull("group_code") ? "" : rows->getString("group_code");
			if (IsValidGroupCode(row.group_code)) {
				if (used_codes.find(row.group_code) == used_codes.end()) {
					used_codes.insert(row.group_code);
				}
				else {
					row.group_code.clear();
					++duplicate_fixed;
				}
			}
			else {
				row.group_code.clear();
			}
			all_rows.push_back(row);
		}

		int backfilled = 0;
		int retries = 0;
		int owner_role_fixed = 0;
		int demoted_extra_owner = 0;
		con->_con->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> update_stmt(
			con->_con->prepareStatement("UPDATE chat_group SET group_code = ? WHERE group_id = ?"));

		for (auto& row : all_rows) {
			if (!row.group_code.empty()) {
				continue;
			}

			std::string new_code;
			for (int i = 0; i < 20; ++i) {
				const std::string candidate = GenerateGroupCode();
				if (used_codes.find(candidate) == used_codes.end()) {
					new_code = candidate;
					break;
				}
				++retries;
			}

			if (new_code.empty()) {
				con->_con->rollback();
				return false;
			}

			update_stmt->setString(1, new_code);
			update_stmt->setInt64(2, row.group_id);
			update_stmt->executeUpdate();
			used_codes.insert(new_code);
			++backfilled;
		}

		std::unique_ptr<sql::PreparedStatement> ensure_owner_stmt(
			con->_con->prepareStatement("INSERT INTO chat_group_member(group_id, uid, role, mute_until, join_source, status) "
				"VALUES(?, ?, 3, 0, 0, 1) "
				"ON DUPLICATE KEY UPDATE role = 3, status = 1, mute_until = 0, updated_at = CURRENT_TIMESTAMP"));
		std::unique_ptr<sql::PreparedStatement> demote_stmt(
			con->_con->prepareStatement("UPDATE chat_group_member SET role = 2, updated_at = CURRENT_TIMESTAMP "
				"WHERE group_id = ? AND uid <> ? AND role = 3 AND status = 1"));
		for (const auto& row : all_rows) {
			std::unique_ptr<sql::PreparedStatement> owner_query(
				con->_con->prepareStatement("SELECT owner_uid FROM chat_group WHERE group_id = ? LIMIT 1"));
			owner_query->setInt64(1, row.group_id);
			std::unique_ptr<sql::ResultSet> owner_res(owner_query->executeQuery());
			if (!owner_res->next()) {
				continue;
			}
			const int owner_uid = owner_res->getInt("owner_uid");
			if (owner_uid <= 0) {
				continue;
			}

			ensure_owner_stmt->setInt64(1, row.group_id);
			ensure_owner_stmt->setInt(2, owner_uid);
			owner_role_fixed += ensure_owner_stmt->executeUpdate();

			demote_stmt->setInt64(1, row.group_id);
			demote_stmt->setInt(2, owner_uid);
			demoted_extra_owner += demote_stmt->executeUpdate();
		}

		con->_con->commit();
		std::cout << "[ChatServer] group_code backfill done. total=" << all_rows.size()
			<< " backfilled=" << backfilled
			<< " duplicate_fixed=" << duplicate_fixed
			<< " retries=" << retries
			<< " owner_role_fixed=" << owner_role_fixed
			<< " demoted_extra_owner=" << demoted_extra_owner << std::endl;
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
		std::cerr << "EnsureGroupCodeSchemaAndBackfill SQLException: " << e.what();
		std::cerr << " (legacy SQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool PostgresDao::GetPendingGroupApplyForReviewer(const int& reviewer_uid, std::vector<std::shared_ptr<GroupApplyInfo>>& applies, int limit) {
	if (use_postgres_) {
		try {
			const int final_limit = std::max(1, std::min(limit, 100));
			pqxx::connection conn(postgres_connection_string_);
			pqxx::read_transaction txn(conn);
			const auto rows = txn.exec_params(
				"SELECT a.apply_id, a.group_id, a.applicant_uid, a.inviter_uid, a.type, a.status, a.reason, a.reviewer_uid "
				"FROM chat_group_apply a "
				"JOIN chat_group_member m ON a.group_id = m.group_id "
				"LEFT JOIN chat_group_admin_permission p ON p.group_id = m.group_id AND p.uid = m.uid "
				"WHERE a.status = 0 AND m.uid = $1 AND m.status = 1 AND "
				"(m.role = 3 OR ((COALESCE(p.permission_bits, $2) & $3) <> 0)) "
				"ORDER BY a.created_at DESC LIMIT $4",
				reviewer_uid,
				kDefaultAdminPermBits,
				kPermInviteUsers,
				final_limit);
			applies.clear();
			for (const auto& row : rows) {
				auto info = std::make_shared<GroupApplyInfo>();
				info->apply_id = row["apply_id"].as<int64_t>();
				info->group_id = row["group_id"].as<int64_t>();
				info->applicant_uid = row["applicant_uid"].is_null() ? 0 : row["applicant_uid"].as<int>();
				info->inviter_uid = row["inviter_uid"].is_null() ? 0 : row["inviter_uid"].as<int>();
				info->type = row["type"].is_null() ? "apply" : ((row["type"].as<int>() == 1) ? "invite" : "apply");
				info->status = row["status"].is_null() ? 0 : row["status"].as<int>();
				info->reason = row["reason"].is_null() ? "" : row["reason"].c_str();
				info->reviewer_uid = row["reviewer_uid"].is_null() ? 0 : row["reviewer_uid"].as<int>();
				applies.push_back(info);
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
		const int final_limit = std::max(1, std::min(limit, 100));
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT a.apply_id, a.group_id, a.applicant_uid, a.inviter_uid, a.type, a.status, a.reason, a.reviewer_uid "
				"FROM chat_group_apply a "
				"JOIN chat_group_member m ON a.group_id = m.group_id "
				"LEFT JOIN chat_group_admin_permission p ON p.group_id = m.group_id AND p.uid = m.uid "
				"WHERE a.status = 0 AND m.uid = ? AND m.status = 1 AND "
				"(m.role = 3 OR ((COALESCE(p.permission_bits, ?) & ?) <> 0)) "
				"ORDER BY a.created_at DESC LIMIT ?"));
		pstmt->setInt(1, reviewer_uid);
		pstmt->setInt64(2, kDefaultAdminPermBits);
		pstmt->setInt64(3, kPermInviteUsers);
		pstmt->setInt(4, final_limit);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto info = std::make_shared<GroupApplyInfo>();
			info->apply_id = static_cast<int64_t>(std::stoll(res->getString("apply_id")));
			info->group_id = static_cast<int64_t>(std::stoll(res->getString("group_id")));
			info->applicant_uid = res->getInt("applicant_uid");
			info->inviter_uid = res->getInt("inviter_uid");
			info->type = (res->getInt("type") == 1) ? "invite" : "apply";
			info->status = res->getInt("status");
			info->reason = res->getString("reason");
			info->reviewer_uid = res->getInt("reviewer_uid");
			applies.push_back(info);
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
