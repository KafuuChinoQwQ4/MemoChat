#include "PrivateChatCacheStore.h"
#include <QDateTime>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <algorithm>

namespace {
QString makeConnectionName(int ownerUid)
{
    return QString("memochat_private_cache_%1").arg(ownerUid);
}
}

PrivateChatCacheStore::PrivateChatCacheStore()
    : _owner_uid(0)
{
}

PrivateChatCacheStore::~PrivateChatCacheStore()
{
    close();
}

bool PrivateChatCacheStore::openForUser(int ownerUid)
{
    if (ownerUid <= 0) {
        return false;
    }
    if (_db.isOpen() && _owner_uid == ownerUid) {
        return true;
    }

    close();

    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty()) {
        return false;
    }
    QDir().mkpath(appDataDir);

    const QString dbPath = appDataDir + QString("/private_chat_%1.db").arg(ownerUid);
    _connection_name = makeConnectionName(ownerUid);
    _db = QSqlDatabase::addDatabase("QSQLITE", _connection_name);
    _db.setDatabaseName(dbPath);
    if (!_db.open()) {
        close();
        return false;
    }
    _owner_uid = ownerUid;
    return ensureSchema();
}

void PrivateChatCacheStore::close()
{
    if (_connection_name.isEmpty()) {
        _db = QSqlDatabase();
        _owner_uid = 0;
        return;
    }

    if (_db.isOpen()) {
        _db.close();
    }
    _db = QSqlDatabase();
    const QString name = _connection_name;
    _connection_name.clear();
    QSqlDatabase::removeDatabase(name);
    _owner_uid = 0;
}

bool PrivateChatCacheStore::isReady() const
{
    return _db.isOpen();
}

std::vector<std::shared_ptr<TextChatData>> PrivateChatCacheStore::loadRecentMessages(int ownerUid, int peerUid, int limit) const
{
    std::vector<std::shared_ptr<TextChatData>> messages;
    if (!_db.isOpen() || ownerUid <= 0 || peerUid <= 0 || limit <= 0) {
        return messages;
    }

    QSqlQuery query(_db);
    query.prepare(
        "SELECT msg_id, content, from_uid, to_uid, created_at, "
        "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms "
        "FROM private_chat_msg "
        "WHERE owner_uid = ? AND peer_uid = ? "
        "ORDER BY created_at DESC, msg_id DESC LIMIT ?");
    query.addBindValue(ownerUid);
    query.addBindValue(peerUid);
    query.addBindValue(limit);

    if (!query.exec()) {
        return messages;
    }

    while (query.next()) {
        const QString msgId = query.value(0).toString();
        const QString content = query.value(1).toString();
        const int fromUid = query.value(2).toInt();
        const int toUid = query.value(3).toInt();
        const qint64 createdAt = query.value(4).toLongLong();
        const qint64 replyToServerMsgId = query.value(5).toLongLong();
        const QString forwardMetaJson = query.value(6).toString();
        qint64 editedAtMs = query.value(7).toLongLong();
        qint64 deletedAtMs = query.value(8).toLongLong();
        if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
            editedAtMs *= 1000;
        }
        if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
            deletedAtMs *= 1000;
        }
        const QString state = deletedAtMs > 0 ? QStringLiteral("deleted")
                           : (editedAtMs > 0 ? QStringLiteral("edited") : QStringLiteral("sent"));
        messages.push_back(std::make_shared<TextChatData>(msgId,
                                                          content,
                                                          fromUid,
                                                          toUid,
                                                          QString(),
                                                          createdAt,
                                                          QString(),
                                                          state,
                                                          0,
                                                          0,
                                                          replyToServerMsgId,
                                                          forwardMetaJson,
                                                          editedAtMs,
                                                          deletedAtMs));
    }

    std::reverse(messages.begin(), messages.end());
    return messages;
}

void PrivateChatCacheStore::upsertMessages(int ownerUid, int peerUid, const std::vector<std::shared_ptr<TextChatData>> &messages)
{
    if (!_db.isOpen() || ownerUid <= 0 || peerUid <= 0 || messages.empty()) {
        return;
    }

    QSqlQuery query(_db);
    query.prepare(
        "INSERT OR REPLACE INTO private_chat_msg("
        "owner_uid, peer_uid, msg_id, from_uid, to_uid, content, created_at, "
        "reply_to_server_msg_id, forward_meta_json, edited_at_ms, deleted_at_ms) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    _db.transaction();
    for (const auto &msg : messages) {
        if (!msg || msg->_msg_id.isEmpty()) {
            continue;
        }
        query.addBindValue(ownerUid);
        query.addBindValue(peerUid);
        query.addBindValue(msg->_msg_id);
        query.addBindValue(msg->_from_uid);
        query.addBindValue(msg->_to_uid);
        query.addBindValue(msg->_msg_content);
        query.addBindValue(normalizeCreatedAt(msg->_created_at));
        query.addBindValue(msg->_reply_to_server_msg_id);
        query.addBindValue(msg->_forward_meta_json);
        query.addBindValue(msg->_edited_at_ms);
        query.addBindValue(msg->_deleted_at_ms);
        if (!query.exec()) {
            _db.rollback();
            return;
        }
    }
    _db.commit();
    pruneConversation(ownerUid, peerUid, 2000);
}

void PrivateChatCacheStore::pruneConversation(int ownerUid, int peerUid, int keepCount)
{
    if (!_db.isOpen() || ownerUid <= 0 || peerUid <= 0 || keepCount <= 0) {
        return;
    }

    QSqlQuery query(_db);
    query.prepare(
        "DELETE FROM private_chat_msg "
        "WHERE owner_uid = ? AND peer_uid = ? AND msg_id IN ("
        "SELECT msg_id FROM private_chat_msg "
        "WHERE owner_uid = ? AND peer_uid = ? "
        "ORDER BY created_at DESC, msg_id DESC LIMIT -1 OFFSET ?)");
    query.addBindValue(ownerUid);
    query.addBindValue(peerUid);
    query.addBindValue(ownerUid);
    query.addBindValue(peerUid);
    query.addBindValue(keepCount);
    query.exec();
}

bool PrivateChatCacheStore::ensureSchema()
{
    if (!_db.isOpen()) {
        return false;
    }

    QSqlQuery query(_db);
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS private_chat_msg ("
            "owner_uid INTEGER NOT NULL,"
            "peer_uid INTEGER NOT NULL,"
            "msg_id TEXT NOT NULL,"
            "from_uid INTEGER NOT NULL,"
            "to_uid INTEGER NOT NULL,"
            "content TEXT NOT NULL,"
            "created_at INTEGER NOT NULL,"
            "reply_to_server_msg_id INTEGER NOT NULL DEFAULT 0,"
            "forward_meta_json TEXT,"
            "edited_at_ms INTEGER NOT NULL DEFAULT 0,"
            "deleted_at_ms INTEGER NOT NULL DEFAULT 0,"
            "PRIMARY KEY(owner_uid, msg_id))")) {
        return false;
    }
    query.exec("ALTER TABLE private_chat_msg ADD COLUMN reply_to_server_msg_id INTEGER NOT NULL DEFAULT 0");
    query.exec("ALTER TABLE private_chat_msg ADD COLUMN forward_meta_json TEXT");
    query.exec("ALTER TABLE private_chat_msg ADD COLUMN edited_at_ms INTEGER NOT NULL DEFAULT 0");
    query.exec("ALTER TABLE private_chat_msg ADD COLUMN deleted_at_ms INTEGER NOT NULL DEFAULT 0");

    if (!query.exec(
            "CREATE INDEX IF NOT EXISTS idx_private_chat_owner_peer_created "
            "ON private_chat_msg(owner_uid, peer_uid, created_at)")) {
        return false;
    }

    return true;
}

qint64 PrivateChatCacheStore::normalizeCreatedAt(qint64 createdAt)
{
    if (createdAt <= 0) {
        return QDateTime::currentMSecsSinceEpoch();
    }
    if (createdAt < 100000000000LL) {
        return createdAt * 1000;
    }
    return createdAt;
}
