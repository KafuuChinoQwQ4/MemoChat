#include "GroupChatCacheStore.h"

#include <QDir>
#include <QSqlQuery>
#include <QStandardPaths>
#include <algorithm>

#include "ChatCacheMessageCodec.h"

namespace
{
QString makeConnectionName(int ownerUid)
{
    return QString("memochat_group_cache_%1").arg(ownerUid);
}
} // namespace

GroupChatCacheStore::GroupChatCacheStore()
    : _owner_uid(0)
{
}

GroupChatCacheStore::~GroupChatCacheStore()
{
    close();
}

bool GroupChatCacheStore::openForUser(int ownerUid)
{
    if (ownerUid <= 0)
    {
        return false;
    }
    if (_db.isOpen() && _owner_uid == ownerUid)
    {
        return true;
    }

    close();

    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty())
    {
        return false;
    }
    QDir().mkpath(appDataDir);

    const QString dbPath = appDataDir + QString("/group_chat_%1.db").arg(ownerUid);
    _connection_name = makeConnectionName(ownerUid);
    _db = QSqlDatabase::addDatabase("QSQLITE", _connection_name);
    _db.setDatabaseName(dbPath);
    if (!_db.open())
    {
        close();
        return false;
    }

    _owner_uid = ownerUid;
    return ensureSchema();
}

void GroupChatCacheStore::close()
{
    if (_connection_name.isEmpty())
    {
        _db = QSqlDatabase();
        _owner_uid = 0;
        return;
    }

    if (_db.isOpen())
    {
        _db.close();
    }
    _db = QSqlDatabase();
    const QString name = _connection_name;
    _connection_name.clear();
    QSqlDatabase::removeDatabase(name);
    _owner_uid = 0;
}

bool GroupChatCacheStore::isReady() const
{
    return _db.isOpen();
}

std::vector<std::shared_ptr<TextChatData>>
GroupChatCacheStore::loadRecentMessages(int ownerUid, qint64 groupId, int limit) const
{
    std::vector<std::shared_ptr<TextChatData>> messages;
    if (!_db.isOpen() || ownerUid <= 0 || groupId <= 0 || limit <= 0)
    {
        return messages;
    }

    QSqlQuery query(_db);
    query.prepare("SELECT msg_id, content, from_uid, from_user_id, from_name, from_icon, created_at, server_msg_id, "
                  "group_seq, "
                  "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms "
                  "FROM group_chat_msg "
                  "WHERE owner_uid = ? AND group_id = ? "
                  "ORDER BY CASE WHEN group_seq > 0 THEN group_seq ELSE created_at END DESC, "
                  "server_msg_id DESC, msg_id DESC LIMIT ?");
    query.addBindValue(ownerUid);
    query.addBindValue(groupId);
    query.addBindValue(limit);

    if (!query.exec())
    {
        return messages;
    }

    while (query.next())
    {
        GroupChatCacheRow row;
        row.msgId = query.value(0).toString();
        row.content = query.value(1).toString();
        row.fromUid = query.value(2).toInt();
        row.fromUserId = query.value(3).toString();
        row.fromName = query.value(4).toString();
        row.fromIcon = query.value(5).toString();
        row.createdAt = query.value(6).toLongLong();
        row.serverMsgId = query.value(7).toLongLong();
        row.groupSeq = query.value(8).toLongLong();
        row.replyToServerMsgId = query.value(9).toLongLong();
        row.forwardMetaJson = query.value(10).toString();
        row.editedAtMs = query.value(11).toLongLong();
        row.deletedAtMs = query.value(12).toLongLong();
        messages.push_back(groupMessageFromCacheRow(row));
    }

    std::reverse(messages.begin(), messages.end());
    return messages;
}

void GroupChatCacheStore::upsertMessages(int ownerUid,
                                         qint64 groupId,
                                         const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    if (!_db.isOpen() || ownerUid <= 0 || groupId <= 0 || messages.empty())
    {
        return;
    }

    QSqlQuery query(_db);
    query.prepare("INSERT OR REPLACE INTO group_chat_msg("
                  "owner_uid, group_id, msg_id, from_uid, from_user_id, content, from_name, from_icon, created_at, "
                  "server_msg_id, "
                  "group_seq, "
                  "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms) "
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    _db.transaction();
    for (const auto& msg : messages)
    {
        if (!msg || msg->_msg_id.isEmpty())
        {
            continue;
        }
        query.addBindValue(ownerUid);
        query.addBindValue(groupId);
        query.addBindValue(msg->_msg_id);
        query.addBindValue(msg->_from_uid);
        query.addBindValue(msg->_from_user_id);
        query.addBindValue(msg->_msg_content);
        query.addBindValue(msg->_from_name);
        query.addBindValue(msg->_from_icon);
        query.addBindValue(normalizeChatCacheTimestamp(msg->_created_at));
        query.addBindValue(msg->_server_msg_id);
        query.addBindValue(msg->_group_seq);
        query.addBindValue(msg->_reply_to_server_msg_id);
        query.addBindValue(msg->_forward_meta_json);
        query.addBindValue(msg->_edited_at_ms);
        query.addBindValue(msg->_deleted_at_ms);
        if (!query.exec())
        {
            _db.rollback();
            return;
        }
    }
    _db.commit();
    pruneConversation(ownerUid, groupId, 2000);
}

void GroupChatCacheStore::pruneConversation(int ownerUid, qint64 groupId, int keepCount)
{
    if (!_db.isOpen() || ownerUid <= 0 || groupId <= 0 || keepCount <= 0)
    {
        return;
    }

    QSqlQuery query(_db);
    query.prepare("DELETE FROM group_chat_msg "
                  "WHERE owner_uid = ? AND group_id = ? AND msg_id IN ("
                  "SELECT msg_id FROM group_chat_msg "
                  "WHERE owner_uid = ? AND group_id = ? "
                  "ORDER BY CASE WHEN group_seq > 0 THEN group_seq ELSE created_at END DESC, "
                  "server_msg_id DESC, msg_id DESC LIMIT -1 OFFSET ?)");
    query.addBindValue(ownerUid);
    query.addBindValue(groupId);
    query.addBindValue(ownerUid);
    query.addBindValue(groupId);
    query.addBindValue(keepCount);
    query.exec();
}

bool GroupChatCacheStore::ensureSchema()
{
    if (!_db.isOpen())
    {
        return false;
    }

    QSqlQuery query(_db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS group_chat_msg ("
                    "owner_uid INTEGER NOT NULL,"
                    "group_id INTEGER NOT NULL,"
                    "msg_id TEXT NOT NULL,"
                    "from_uid INTEGER NOT NULL,"
                    "from_user_id TEXT,"
                    "content TEXT NOT NULL,"
                    "from_name TEXT,"
                    "from_icon TEXT,"
                    "created_at INTEGER NOT NULL,"
                    "server_msg_id INTEGER NOT NULL DEFAULT 0,"
                    "group_seq INTEGER NOT NULL DEFAULT 0,"
                    "reply_to_server_msg_id INTEGER NOT NULL DEFAULT 0,"
                    "forward_meta_json TEXT,"
                    "edited_at_ms INTEGER NOT NULL DEFAULT 0,"
                    "deleted_at_ms INTEGER NOT NULL DEFAULT 0,"
                    "PRIMARY KEY(owner_uid, msg_id))"))
    {
        return false;
    }

    query.exec("ALTER TABLE group_chat_msg ADD COLUMN server_msg_id INTEGER NOT NULL DEFAULT 0");
    query.exec("ALTER TABLE group_chat_msg ADD COLUMN group_seq INTEGER NOT NULL DEFAULT 0");
    query.exec("ALTER TABLE group_chat_msg ADD COLUMN reply_to_server_msg_id INTEGER NOT NULL DEFAULT 0");
    query.exec("ALTER TABLE group_chat_msg ADD COLUMN forward_meta_json TEXT");
    query.exec("ALTER TABLE group_chat_msg ADD COLUMN edited_at_ms INTEGER NOT NULL DEFAULT 0");
    query.exec("ALTER TABLE group_chat_msg ADD COLUMN deleted_at_ms INTEGER NOT NULL DEFAULT 0");
    query.exec("ALTER TABLE group_chat_msg ADD COLUMN from_user_id TEXT");

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_group_chat_owner_group_created "
                    "ON group_chat_msg(owner_uid, group_id, created_at)"))
    {
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_group_chat_owner_group_seq "
                    "ON group_chat_msg(owner_uid, group_id, group_seq)"))
    {
        return false;
    }

    return true;
}
