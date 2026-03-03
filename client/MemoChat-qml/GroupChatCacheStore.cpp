#include "GroupChatCacheStore.h"
#include <QDateTime>
#include <QDir>
#include <QSqlQuery>
#include <QStandardPaths>
#include <algorithm>

namespace {
QString makeConnectionName(int ownerUid)
{
    return QString("memochat_group_cache_%1").arg(ownerUid);
}
}

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

    const QString dbPath = appDataDir + QString("/group_chat_%1.db").arg(ownerUid);
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

void GroupChatCacheStore::close()
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

bool GroupChatCacheStore::isReady() const
{
    return _db.isOpen();
}

std::vector<std::shared_ptr<TextChatData>> GroupChatCacheStore::loadRecentMessages(int ownerUid, qint64 groupId, int limit) const
{
    std::vector<std::shared_ptr<TextChatData>> messages;
    if (!_db.isOpen() || ownerUid <= 0 || groupId <= 0 || limit <= 0) {
        return messages;
    }

    QSqlQuery query(_db);
    query.prepare(
        "SELECT msg_id, content, from_uid, from_name, from_icon, created_at "
        "FROM group_chat_msg "
        "WHERE owner_uid = ? AND group_id = ? "
        "ORDER BY created_at DESC, msg_id DESC LIMIT ?");
    query.addBindValue(ownerUid);
    query.addBindValue(groupId);
    query.addBindValue(limit);

    if (!query.exec()) {
        return messages;
    }

    while (query.next()) {
        const QString msgId = query.value(0).toString();
        const QString content = query.value(1).toString();
        const int fromUid = query.value(2).toInt();
        const QString fromName = query.value(3).toString();
        const QString fromIcon = query.value(4).toString();
        const qint64 createdAt = normalizeCreatedAt(query.value(5).toLongLong());
        messages.push_back(std::make_shared<TextChatData>(msgId, content, fromUid, 0, fromName, createdAt, fromIcon));
    }

    std::reverse(messages.begin(), messages.end());
    return messages;
}

void GroupChatCacheStore::upsertMessages(int ownerUid, qint64 groupId, const std::vector<std::shared_ptr<TextChatData>> &messages)
{
    if (!_db.isOpen() || ownerUid <= 0 || groupId <= 0 || messages.empty()) {
        return;
    }

    QSqlQuery query(_db);
    query.prepare(
        "INSERT OR REPLACE INTO group_chat_msg("
        "owner_uid, group_id, msg_id, from_uid, content, from_name, from_icon, created_at) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?)");

    _db.transaction();
    for (const auto &msg : messages) {
        if (!msg || msg->_msg_id.isEmpty()) {
            continue;
        }
        query.addBindValue(ownerUid);
        query.addBindValue(groupId);
        query.addBindValue(msg->_msg_id);
        query.addBindValue(msg->_from_uid);
        query.addBindValue(msg->_msg_content);
        query.addBindValue(msg->_from_name);
        query.addBindValue(msg->_from_icon);
        query.addBindValue(normalizeCreatedAt(msg->_created_at));
        if (!query.exec()) {
            _db.rollback();
            return;
        }
    }
    _db.commit();
    pruneConversation(ownerUid, groupId, 2000);
}

void GroupChatCacheStore::pruneConversation(int ownerUid, qint64 groupId, int keepCount)
{
    if (!_db.isOpen() || ownerUid <= 0 || groupId <= 0 || keepCount <= 0) {
        return;
    }

    QSqlQuery query(_db);
    query.prepare(
        "DELETE FROM group_chat_msg "
        "WHERE owner_uid = ? AND group_id = ? AND msg_id IN ("
        "SELECT msg_id FROM group_chat_msg "
        "WHERE owner_uid = ? AND group_id = ? "
        "ORDER BY created_at DESC, msg_id DESC LIMIT -1 OFFSET ?)");
    query.addBindValue(ownerUid);
    query.addBindValue(groupId);
    query.addBindValue(ownerUid);
    query.addBindValue(groupId);
    query.addBindValue(keepCount);
    query.exec();
}

bool GroupChatCacheStore::ensureSchema()
{
    if (!_db.isOpen()) {
        return false;
    }

    QSqlQuery query(_db);
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS group_chat_msg ("
            "owner_uid INTEGER NOT NULL,"
            "group_id INTEGER NOT NULL,"
            "msg_id TEXT NOT NULL,"
            "from_uid INTEGER NOT NULL,"
            "content TEXT NOT NULL,"
            "from_name TEXT,"
            "from_icon TEXT,"
            "created_at INTEGER NOT NULL,"
            "PRIMARY KEY(owner_uid, msg_id))")) {
        return false;
    }

    if (!query.exec(
            "CREATE INDEX IF NOT EXISTS idx_group_chat_owner_group_created "
            "ON group_chat_msg(owner_uid, group_id, created_at)")) {
        return false;
    }

    return true;
}

qint64 GroupChatCacheStore::normalizeCreatedAt(qint64 createdAt)
{
    if (createdAt <= 0) {
        return QDateTime::currentMSecsSinceEpoch();
    }
    if (createdAt < 100000000000LL) {
        return createdAt * 1000;
    }
    return createdAt;
}
