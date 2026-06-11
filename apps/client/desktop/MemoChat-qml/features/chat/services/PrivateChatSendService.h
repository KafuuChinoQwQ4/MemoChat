#ifndef PRIVATECHATSENDSERVICE_H
#define PRIVATECHATSENDSERVICE_H

#include "ChatDispatchService.h"

#include <QString>
#include <functional>
#include <memory>
#include <vector>

class ChatMessageModel;
class FriendListModel;
class PrivateChatCacheStore;
struct TextChatData;

struct PrivateChatSendRequest
{
    int peerUid = 0;
    QString content;
    QString previewText;
};

struct PrivateChatSendDependencies
{
    int selfUid = 0;
    std::function<bool(const OutgoingChatPacket&, QString*)> dispatchPacket;
    std::function<void(int, const std::vector<std::shared_ptr<TextChatData>>&)> appendFriendMessages;
    PrivateChatCacheStore* cacheStore = nullptr;
    ChatMessageModel* messageModel = nullptr;
    FriendListModel* chatListModel = nullptr;
    FriendListModel* dialogListModel = nullptr;
    qint64* historyBeforeTs = nullptr;
    QString* historyBeforeMsgId = nullptr;
};

struct PrivateChatSendResult
{
    bool success = false;
    QString errorText;
    OutgoingChatPacket packet;
    std::shared_ptr<TextChatData> optimisticMessage;
};

class PrivateChatSendService
{
public:
    static PrivateChatSendResult send(const PrivateChatSendRequest& request,
                                      const PrivateChatSendDependencies& dependencies);
    static bool trySend(const PrivateChatSendRequest& request,
                        const PrivateChatSendDependencies& dependencies,
                        QString* errorText = nullptr);
};

#endif // PRIVATECHATSENDSERVICE_H
