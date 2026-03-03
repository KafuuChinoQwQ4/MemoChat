#ifndef PRIVATECHATCACHESTORE_H
#define PRIVATECHATCACHESTORE_H

#include <QString>
#include <QSqlDatabase>
#include <memory>
#include <vector>
#include "userdata.h"

class PrivateChatCacheStore
{
public:
    PrivateChatCacheStore();
    ~PrivateChatCacheStore();

    bool openForUser(int ownerUid);
    void close();
    bool isReady() const;

    std::vector<std::shared_ptr<TextChatData>> loadRecentMessages(int ownerUid, int peerUid, int limit) const;
    void upsertMessages(int ownerUid, int peerUid, const std::vector<std::shared_ptr<TextChatData>> &messages);
    void pruneConversation(int ownerUid, int peerUid, int keepCount = 2000);

private:
    bool ensureSchema();
    static qint64 normalizeCreatedAt(qint64 createdAt);

    QString _connection_name;
    QSqlDatabase _db;
    int _owner_uid;
};

#endif // PRIVATECHATCACHESTORE_H
