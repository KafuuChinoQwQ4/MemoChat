#ifndef GROUPCHATCACHESTORE_H
#define GROUPCHATCACHESTORE_H

#include <QString>
#include <QSqlDatabase>
#include <memory>
#include <vector>
#include "userdata.h"

class GroupChatCacheStore
{
public:
    GroupChatCacheStore();
    ~GroupChatCacheStore();

    bool openForUser(int ownerUid);
    void close();
    bool isReady() const;

    std::vector<std::shared_ptr<TextChatData>> loadRecentMessages(int ownerUid, qint64 groupId, int limit) const;
    void upsertMessages(int ownerUid, qint64 groupId, const std::vector<std::shared_ptr<TextChatData>> &messages);
    void pruneConversation(int ownerUid, qint64 groupId, int keepCount = 2000);

private:
    bool ensureSchema();
    static qint64 normalizeCreatedAt(qint64 createdAt);

    QString _connection_name;
    QSqlDatabase _db;
    int _owner_uid;
};

#endif // GROUPCHATCACHESTORE_H
