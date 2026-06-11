#ifndef CHATDIALOGLISTRESPONSESERVICE_H
#define CHATDIALOGLISTRESPONSESERVICE_H

#include "DialogListTypes.h"

#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <vector>

struct FriendInfo;
struct GroupInfoData;

struct ChatDialogListResponseContext
{
    bool bootstrappingDialog = false;
    bool hasCurrentDialog = false;
    int currentPrivatePeerUid = 0;
    qint64 currentGroupId = 0;
    std::vector<std::shared_ptr<FriendInfo>> friendSnapshot;
    std::vector<std::shared_ptr<GroupInfoData>> groupSnapshot;
    std::vector<QVariantMap> existingDialogs;
    DialogDecorationState decorationState;
};

struct ChatDialogServerMetaSeed
{
    int dialogUid = 0;
    int pinnedRank = 0;
    int muteState = 0;
};

struct ChatDialogListResponseEffect
{
    bool dialogsReady = false;
    bool chatListInitialized = false;
    bool resetServerDialogMeta = false;
    QVector<ChatDialogServerMetaSeed> serverMetaSeeds;
    std::vector<std::shared_ptr<FriendInfo>> upsertChatDialogs;
    std::vector<std::shared_ptr<FriendInfo>> upsertGroupDialogs;
    std::vector<std::shared_ptr<FriendInfo>> replaceDialogList;
    std::vector<std::shared_ptr<GroupInfoData>> upsertGroups;
    QSet<int> privateDialogUids;
    QSet<qint64> groupDialogIds;
    QVector<int> clearUnreadDialogUids;
    QVector<int> clearMentionDialogUids;
    int selectDialogUid = 0;
    bool selectFirstChat = false;
    QVector<int> bootstrapPrivateHistoryUids;
    QVector<qint64> bootstrapGroupHistoryIds;
    int currentPrivateHistoryUid = 0;
    bool syncCurrentDialogDraft = false;
    bool refreshDialogModel = false;
    bool flushIncomingMessages = false;
};

struct ChatDialogListResponsePort
{
    std::function<void()> resetServerDialogMeta;
    std::function<void(int, int, int)> seedServerDialogMeta;
    std::function<void(std::shared_ptr<FriendInfo>)> upsertChatDialog;
    std::function<void(const std::vector<std::shared_ptr<FriendInfo>>&)> replaceDialogList;
    std::function<void(int)> clearUnreadDialog;
    std::function<void(int)> clearMentionDialog;
    std::function<void(int)> removeMentionForDialog;
};

struct ChatDialogListAppPort
{
    std::function<void(bool)> setDialogBootstrapLoading;
    std::function<void(bool)> setDialogsReady;
    std::function<void(bool)> setChatListInitialized;
    std::function<void(std::shared_ptr<GroupInfoData>)> upsertGroup;
    std::function<void(std::shared_ptr<FriendInfo>)> upsertGroupDialog;
    std::function<void()> refreshDialogModel;
    std::function<void()> syncCurrentDialogDraft;
    std::function<void(int)> selectDialogByUid;
    std::function<void()> selectFirstChat;
    std::function<void(int)> requestPrivateHistoryForBootstrap;
    std::function<void(int)> requestCurrentPrivateHistory;
    std::function<void(qint64)> requestGroupHistoryForBootstrap;
    std::function<void()> flushIncomingMessages;
};

class ChatDialogListResponseService
{
public:
    static ChatDialogListResponseEffect
    reduce(const QJsonObject& payload, ChatDialogListResponseContext context, QMap<int, qint64>& groupDialogUidMap);
    static void apply(const ChatDialogListResponseEffect& effect,
                      const ChatDialogListResponsePort& featurePort,
                      const ChatDialogListAppPort& appPort);
    static ChatDialogListResponseEffect handle(const QJsonObject& payload,
                                               ChatDialogListResponseContext context,
                                               QMap<int, qint64>& groupDialogUidMap,
                                               const ChatDialogListResponsePort& featurePort,
                                               const ChatDialogListAppPort& appPort);
};

#endif // CHATDIALOGLISTRESPONSESERVICE_H
