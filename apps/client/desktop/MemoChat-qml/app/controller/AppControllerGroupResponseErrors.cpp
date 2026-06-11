#include "AppController.h"
#include "ChatEventDependenciesFactory.h"
#include "GroupManagementEffectPortFactory.h"
#include "IChatTransport.h"
#include "PrivateChatEventService.h"
#include "GroupResponseErrorService.h"
#include "usermgr.h"

#include <QJsonObject>

namespace
{
QString responseActionText(int reqId)
{
    const QString privateAction = PrivateChatEventService::privateMessageResponseActionText(static_cast<ReqId>(reqId));
    if (!privateAction.isEmpty())
    {
        return privateAction;
    }

    const QString dialogMetaAction = ChatFeatureController::dialogMetaActionText(reqId);
    if (!dialogMetaAction.isEmpty())
    {
        return dialogMetaAction;
    }

    return {};
}
} // namespace

bool AppController::handleGroupRspError(ReqId reqId, int error, const QJsonObject& payload)
{
    if (reqId == ID_GROUP_HISTORY_RSP)
    {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        if (groupId <= 0 || groupId == currentGroupId())
        {
            _shell_state.loadingState().groupHistoryLoading = false;
            setPrivateHistoryLoading(false);
        }
    }
    if (reqId == ID_GROUP_CHAT_MSG_RSP)
    {
        GroupAckRequest request;
        request.context = groupConversationContext(_gateway.userMgr()->GetUid());
        request.reqId = reqId;
        request.errorCode = error;
        request.payload = payload;
        const GroupAckResult result = _features.chatFeatureController.handleGroupMessageError(
            request,
            memochat::app::makeGroupConversationDependencies(
                _features.chatFeatureController,
                _gateway.userMgr(),
                _features.groupController.groupListModel(),
                [this](int dispatchReqId, const QByteArray& dispatchPayload)
                {
                    if (const auto transport = _gateway.chatTransport())
                    {
                        transport->slot_send_data(static_cast<ReqId>(dispatchReqId), dispatchPayload);
                    }
                }));
        if (!result.statusText.isEmpty())
        {
            setGroupStatus(result.statusText, result.statusIsError);
        }
        return true;
    }
    if (reqId == ID_SYNC_DRAFT_RSP)
    {
        return true;
    }

    const memochat::group::GroupResponseErrorDecision decision =
        memochat::group::GroupResponseErrorService::reduce(reqId, error, payload, responseActionText);
    switch (decision.target)
    {
        case memochat::group::GroupResponseErrorTarget::Tip:
            setTip(decision.statusText, decision.statusIsError);
            break;
        case memochat::group::GroupResponseErrorTarget::ManagementEffect:
            _features.groupController.applyGroupManagementResponseEffect(
                decision.managementEffect,
                memochat::app::makeGroupManagementResponseEffectPort(groupManagementResponseEffectActions()));
            break;
        case memochat::group::GroupResponseErrorTarget::GroupStatus:
            setGroupStatus(decision.statusText, decision.statusIsError);
            break;
        case memochat::group::GroupResponseErrorTarget::Ignore:
            break;
    }
    return true;
}
