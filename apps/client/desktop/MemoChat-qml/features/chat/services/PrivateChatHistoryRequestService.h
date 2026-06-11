#ifndef PRIVATECHATHISTORYREQUESTSERVICE_H
#define PRIVATECHATHISTORYREQUESTSERVICE_H

#include "PrivateChatHistoryService.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

class ChatMessageModel;
struct TextChatData;

struct PrivateHistoryPeerProfile
{
    bool available = false;
    QString displayName;
    QString icon;
};

struct PrivateHistoryLoadCurrentRequest
{
    int currentPeerUid = 0;
    int selfUid = 0;
    int localLimit = 20;
};

struct PrivateHistoryLoadCurrentDependencies
{
    PrivateHistoryRequestState* historyState = nullptr;
    ChatMessageModel* messageModel = nullptr;
    QString defaultPeerIcon = QStringLiteral("qrc:/res/head_1.png");
    std::function<PrivateHistoryPeerProfile(int)> peerProfile;
    std::function<std::vector<std::shared_ptr<TextChatData>>(int)> friendMessages;
    std::function<std::vector<std::shared_ptr<TextChatData>>(int, int, int)> loadRecentMessages;
    std::function<void(int, const std::vector<std::shared_ptr<TextChatData>>&)> appendFriendMessages;
    std::function<void(const QString&)> setCurrentPeerName;
    std::function<void(const QString&)> setCurrentPeerIcon;
    std::function<void(bool)> setLoading;
    std::function<void(bool)> setCanLoadMore;
    std::function<void(int, qint64)> requestReadAck;
};

struct PrivateHistoryLoadCurrentResult
{
    bool success = false;
    bool cleared = false;
    bool shouldRequestInitialHistory = false;
    QString errorText;
    int peerUid = 0;
    int selfUid = 0;
    int localCacheCount = 0;
    int friendMessageCount = 0;
    qint64 beforeTs = 0;
    QString beforeMsgId;
    qint64 requestedReadAckTs = 0;
};

struct PrivateHistoryRequestBuildRequest
{
    int peerUid = 0;
    int selfUid = 0;
    qint64 beforeTs = 0;
    QString beforeMsgId;
    bool privateHistoryLoading = false;
};

struct PrivateHistoryRequestBuildDependencies
{
    PrivateHistoryRequestState* historyState = nullptr;
    std::function<void(bool)> setLoading;
    std::function<void(int, const QByteArray&)> dispatchPayload;
};

struct PrivateHistoryRequestBuildResult
{
    bool success = false;
    bool pendingUpdated = false;
    bool loadingSet = false;
    bool dispatched = false;
    QString errorText;
    QJsonObject payload;
    QByteArray compactPayload;
    int peerUid = 0;
    int selfUid = 0;
    qint64 beforeTs = 0;
    QString beforeMsgId;
};

struct PrivateHistoryBootstrapRequest
{
    int peerUid = 0;
    int selfUid = 0;
};

struct PrivateHistoryBootstrapDependencies
{
    std::function<void(int, const QByteArray&)> dispatchPayload;
};

struct PrivateHistoryLoadMoreRequest
{
    int currentPeerUid = 0;
    int selfUid = 0;
    bool privateHistoryLoading = false;
    bool canLoadMorePrivateHistory = false;
};

struct PrivateHistoryLoadMoreDependencies
{
    PrivateHistoryRequestState* historyState = nullptr;
    const ChatMessageModel* messageModel = nullptr;
    std::function<void(bool)> setLoading;
    std::function<void(int, const QByteArray&)> dispatchPayload;
};

struct PrivateHistoryLoadMoreCursor
{
    bool shouldRequest = false;
    bool usedModelCursor = false;
    bool usedStateCursor = false;
    QString errorText;
    int peerUid = 0;
    qint64 beforeTs = 0;
    QString beforeMsgId;
};

class PrivateChatHistoryRequestService
{
public:
    static constexpr int kHistoryLimit = 20;

    static void clearState(PrivateHistoryRequestState& state);
    static void clearPending(PrivateHistoryRequestState& state);

    static PrivateHistoryLoadCurrentResult loadCurrent(const PrivateHistoryLoadCurrentRequest& request,
                                                       const PrivateHistoryLoadCurrentDependencies& dependencies);

    static PrivateHistoryRequestBuildResult buildRequest(const PrivateHistoryRequestBuildRequest& request,
                                                         const PrivateHistoryRequestBuildDependencies& dependencies);

    static PrivateHistoryRequestBuildResult
    buildBootstrapRequest(const PrivateHistoryBootstrapRequest& request,
                          const PrivateHistoryBootstrapDependencies& dependencies = {});

    static PrivateHistoryLoadMoreCursor selectLoadMoreCursor(const PrivateHistoryLoadMoreRequest& request,
                                                             const PrivateHistoryLoadMoreDependencies& dependencies);

    static PrivateHistoryRequestBuildResult
    buildLoadMoreRequest(const PrivateHistoryLoadMoreRequest& request,
                         const PrivateHistoryLoadMoreDependencies& dependencies);
};

#endif // PRIVATECHATHISTORYREQUESTSERVICE_H
