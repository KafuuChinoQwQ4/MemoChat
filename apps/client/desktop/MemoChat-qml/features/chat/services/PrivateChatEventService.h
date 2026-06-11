#ifndef PRIVATECHATEVENTSERVICE_H
#define PRIVATECHATEVENTSERVICE_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QtGlobal>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

class ChatMessageModel;
class FriendListModel;
class PrivateChatCacheStore;
struct FriendInfo;
struct AuthInfo;
struct TextChatData;
struct TextChatMsg;

struct PrivateChatEventContext
{
    int selfUid = 0;
    int currentPeerUid = 0;
    qint64 currentGroupId = 0;
};

struct PrivateChatEventDependencies
{
    PrivateChatCacheStore* cacheStore = nullptr;
    ChatMessageModel* messageModel = nullptr;
    FriendListModel* chatListModel = nullptr;
    FriendListModel* dialogListModel = nullptr;
    qint64* historyBeforeTs = nullptr;
    QString* historyBeforeMsgId = nullptr;
    std::function<std::shared_ptr<FriendInfo>(int)> friendById;
    std::function<std::shared_ptr<FriendInfo>(int)> ensureFriend;
    std::function<void(std::shared_ptr<AuthInfo>)> addFriend;
    std::function<void(std::shared_ptr<AuthInfo>)> upsertContact;
    std::function<void(int, const std::vector<std::shared_ptr<TextChatData>>&)> appendFriendMessages;
    std::function<bool(int, const QString&, const QString&, const QString&, qint64, qint64)>
        updatePrivateMessageContent;
    std::function<int(int, int, qint64)> markOutgoingReadUntil;
    std::function<bool(int, const QString&, const QString&)> updatePrivateMessageState;
    std::function<void(int, qint64)> requestReadAck;
};

struct PrivateIncomingMessageRequest
{
    PrivateChatEventContext context;
    std::shared_ptr<TextChatMsg> message;
};

struct PrivateIncomingMessageResult
{
    bool success = false;
    int peerUid = 0;
    bool fromSelf = false;
    bool currentDialog = false;
    std::size_t messageCount = 0;
    bool ensuredFriend = false;
    bool friendMessagesAppended = false;
    bool cacheUpdated = false;
    bool previewUpdated = false;
    bool unreadCleared = false;
    bool unreadIncremented = false;
    std::size_t modelAppendCount = 0;
    qint64 requestedReadAckTs = 0;
};

struct PrivateMessageChangedRequest
{
    PrivateChatEventContext context;
    QJsonObject payload;
};

struct PrivateMessageChangedResult
{
    bool success = false;
    int reqId = 0;
    int peerUid = 0;
    QString msgId;
    QString event;
    QString content;
    QString state;
    qint64 editedAtMs = 0;
    qint64 deletedAtMs = 0;
    bool userManagerUpdated = false;
    bool cacheUpdated = false;
    bool previewUpdated = false;
    bool modelPatched = false;
    QString statusText;
    bool statusIsError = false;
};

struct PrivateMessageMutationResponseRequest
{
    PrivateChatEventContext context;
    int reqId = 0;
    QJsonObject payload;
};

struct PrivateForwardResponseRequest
{
    PrivateChatEventContext context;
    QJsonObject payload;
};

struct PrivateForwardResponseResult
{
    bool success = false;
    int peerUid = 0;
    QString msgId;
    QString content;
    qint64 createdAt = 0;
    bool userManagerUpdated = false;
    bool cacheUpdated = false;
    bool previewUpdated = false;
    bool modelUpserted = false;
    QString statusText;
    bool isError = false;
};

struct PrivateReadAckRequest
{
    PrivateChatEventContext context;
    QJsonObject payload;
};

struct PrivateReadAckResult
{
    bool success = false;
    int peerUid = 0;
    qint64 readTs = 0;
    int updatedCount = 0;
    bool cacheUpdated = false;
    int modelPatchCount = 0;
};

struct PrivateMessageStatusRequest
{
    PrivateChatEventContext context;
    QJsonObject payload;
};

struct PrivateMessageStatusResult
{
    bool success = false;
    int peerUid = 0;
    QString clientMsgId;
    QString nextState;
    bool userManagerUpdated = false;
    bool cacheUpdated = false;
    bool modelPatched = false;
};

class PrivateChatEventService
{
public:
    static PrivateIncomingMessageResult handleIncomingMessage(const PrivateIncomingMessageRequest& request,
                                                              const PrivateChatEventDependencies& dependencies);
    static PrivateMessageChangedResult handleMessageChanged(const PrivateMessageChangedRequest& request,
                                                            const PrivateChatEventDependencies& dependencies);
    static PrivateMessageChangedResult
    handleMessageMutationResponse(const PrivateMessageMutationResponseRequest& request,
                                  const PrivateChatEventDependencies& dependencies);
    static bool isPrivateMessageResponse(int reqId);
    static QString privateMessageResponseActionText(int reqId);
    static PrivateForwardResponseResult handleForwardResponse(const PrivateForwardResponseRequest& request,
                                                              const PrivateChatEventDependencies& dependencies);
    static PrivateReadAckResult handleReadAck(const PrivateReadAckRequest& request,
                                              const PrivateChatEventDependencies& dependencies);
    static PrivateMessageStatusResult handleMessageStatus(const PrivateMessageStatusRequest& request,
                                                          const PrivateChatEventDependencies& dependencies);
    static QByteArray buildReadAckPayload(int selfUid, int peerUid, qint64 readTs);
    static QString resolveMessageStatusState(const QJsonObject& payload);
};

#endif // PRIVATECHATEVENTSERVICE_H
