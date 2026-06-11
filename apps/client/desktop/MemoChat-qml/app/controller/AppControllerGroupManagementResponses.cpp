#include "AppController.h"
#include "ChatEventDependenciesFactory.h"
#include "GroupManagementEffectPortFactory.h"
#include "GroupManagementResponseService.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QJsonObject>

memochat::app::GroupManagementEventEffectActions AppController::groupManagementEventEffectActions()
{
    memochat::app::GroupManagementEventEffectActions actions;
    actions.removeGroup = [this](qint64 groupId)
    {
        _gateway.userMgr()->RemoveGroup(groupId);
    };
    actions.clearGroupConversation = [this](qint64 groupId)
    {
        clearCurrentGroupConversation(groupId);
    };
    actions.refreshGroupModel = [this]()
    {
        refreshGroupModel();
    };
    actions.refreshDialogModel = [this]()
    {
        refreshDialogModel();
    };
    actions.requestDialogList = [this]()
    {
        requestDialogList();
    };
    actions.selectCurrentGroup = [this](qint64 groupId, const QString& name, const QString& groupCode)
    {
        setCurrentGroup(groupId, name, groupCode);
    };
    actions.clearCurrentGroup = [this]()
    {
        setCurrentGroup(0, QString());
    };
    actions.setCurrentChatPeerIcon = [this](const QString& icon)
    {
        setCurrentChatPeerIcon(icon);
    };
    actions.openGroupConversation = [this](qint64 groupId, bool resetHistory)
    {
        GroupOpenRequest request;
        request.context = groupConversationContext(_gateway.userMgr()->GetUid());
        request.groupId = groupId;
        request.resetHistory = resetHistory;
        _features.chatFeatureController.openGroupConversation(
            request,
            memochat::app::makeGroupConversationDependencies(
                _features.chatFeatureController,
                _gateway.userMgr(),
                _features.groupController.groupListModel(),
                [this](int reqId, const QByteArray& payload)
                {
                    if (const auto transport = _gateway.chatTransport())
                    {
                        transport->slot_send_data(static_cast<ReqId>(reqId), payload);
                    }
                }));
    };
    actions.syncCurrentDialogDraft = [this]()
    {
        syncCurrentDialogDraft();
    };
    actions.loadCurrentChatMessages = [this]()
    {
        loadCurrentChatMessages();
    };
    actions.clearMessageModel = [this]()
    {
        _features.chatFeatureController.clearMessageModel();
    };
    actions.resetCurrentChatProjection = [this]()
    {
        setCurrentChatPeerName(QString());
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
    };
    return actions;
}

memochat::app::GroupManagementResponseEffectActions AppController::groupManagementResponseEffectActions()
{
    memochat::app::GroupManagementResponseEffectActions actions;
    actions.removeGroup = [this](qint64 groupId)
    {
        _gateway.userMgr()->RemoveGroup(groupId);
    };
    actions.clearGroupConversation = [this](qint64 groupId)
    {
        clearCurrentGroupConversation(groupId);
    };
    actions.refreshGroupModel = [this]()
    {
        refreshGroupModel();
    };
    actions.refreshDialogModel = [this]()
    {
        refreshDialogModel();
    };
    actions.selectDialogByUid = [this](int dialogUid)
    {
        selectDialogByUid(dialogUid);
    };
    actions.setCurrentChatPeerIcon = [this](const QString& icon)
    {
        setCurrentChatPeerIcon(icon);
    };
    return actions;
}

void AppController::handleGroupManagementRsp(ReqId reqId, const QJsonObject& payload)
{
    memochat::group::GroupManagementResponseContext context;
    context.currentGroupId = currentGroupId();
    const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
    if (groupId > 0)
    {
        refreshGroupModel();
        context.dialogUidForCreatedGroup = _features.chatFeatureController.resolveGroupDialogUid(groupId);
    }
    _features.groupController.applyGroupManagementResponseEffect(
        memochat::group::GroupManagementResponseService::reduceSuccess(reqId, payload, context),
        memochat::app::makeGroupManagementResponseEffectPort(groupManagementResponseEffectActions()));
}
