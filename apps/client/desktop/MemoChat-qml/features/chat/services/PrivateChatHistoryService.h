#ifndef PRIVATECHATHISTORYSERVICE_H
#define PRIVATECHATHISTORYSERVICE_H

#include <QJsonObject>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

class ChatMessageModel;
class FriendListModel;
class PrivateChatCacheStore;
struct TextChatData;

struct PrivateHistoryRequestState
{
    qint64 beforeTs = 0;
    QString beforeMsgId;
    qint64 pendingBeforeTs = 0;
    QString pendingBeforeMsgId;
    int pendingPeerUid = 0;
};

struct PrivateHistoryResponseRequest
{
    QJsonObject payload;
    int currentPeerUid = 0;
    int selfUid = 0;
};

struct PrivateHistoryResponseDependencies
{
    PrivateHistoryRequestState* historyState = nullptr;
    PrivateChatCacheStore* cacheStore = nullptr;
    ChatMessageModel* messageModel = nullptr;
    FriendListModel* chatListModel = nullptr;
    FriendListModel* dialogListModel = nullptr;
    std::function<void(bool)> setLoading;
    std::function<void(bool)> setCanLoadMore;
    std::function<void(int, const std::vector<std::shared_ptr<TextChatData>>&)> appendFriendMessages;
    std::function<std::vector<std::shared_ptr<TextChatData>>(int)> friendMessages;
    std::function<void(int, qint64)> requestReadAck;
};

struct PrivateHistoryResponseResult
{
    int peerUid = 0;
    bool pendingMatched = false;
    bool success = false;
    bool currentDialog = false;
    bool pagination = false;
    bool canLoadMore = false;
    std::size_t parsedCount = 0;
    qint64 requestedReadAckTs = 0;
};

class PrivateChatHistoryService
{
public:
    static PrivateHistoryResponseResult handleResponse(const PrivateHistoryResponseRequest& request,
                                                       const PrivateHistoryResponseDependencies& dependencies);
};

#endif // PRIVATECHATHISTORYSERVICE_H
