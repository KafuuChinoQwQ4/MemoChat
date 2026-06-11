#include "GroupResponseErrorService.h"

#include "global.h"

namespace memochat::group
{
namespace
{
QString fallbackActionText(int reqId, const std::function<QString(int)>& actionText)
{
    if (actionText)
    {
        const QString text = actionText(reqId).trimmed();
        if (!text.isEmpty())
        {
            return text;
        }
    }
    return GroupManagementResponseService::responseActionText(reqId);
}

QString errorStatusText(int reqId, int error, const QJsonObject& payload, const std::function<QString(int)>& actionText)
{
    const QString serverMessage = payload.value(QStringLiteral("message")).toString().trimmed();
    const QString action = fallbackActionText(reqId, actionText);
    return serverMessage.isEmpty() ? QStringLiteral("%1失败（错误码:%2）").arg(action).arg(error)
                                   : QStringLiteral("%1失败：%2（错误码:%3）").arg(action, serverMessage).arg(error);
}
} // namespace

GroupResponseErrorDecision GroupResponseErrorService::reduce(int reqId,
                                                             int error,
                                                             const QJsonObject& payload,
                                                             const std::function<QString(int)>& actionText)
{
    GroupResponseErrorDecision decision;

    if (reqId == ID_SYNC_DRAFT_RSP)
    {
        decision.target = GroupResponseErrorTarget::Ignore;
        decision.handled = true;
        return decision;
    }

    if (reqId == ID_EDIT_PRIVATE_MSG_RSP || reqId == ID_REVOKE_PRIVATE_MSG_RSP || reqId == ID_FORWARD_PRIVATE_MSG_RSP)
    {
        decision.target = GroupResponseErrorTarget::Tip;
        decision.statusIsError = true;
        decision.statusText = errorStatusText(reqId, error, payload, actionText);
        decision.handled = true;
        return decision;
    }

    if (GroupManagementResponseService::isManagementResponse(reqId))
    {
        decision.target = GroupResponseErrorTarget::ManagementEffect;
        decision.managementEffect = GroupManagementResponseService::reduceError(reqId, error, payload);
        decision.handled = decision.managementEffect.handled;
        return decision;
    }

    decision.target = GroupResponseErrorTarget::GroupStatus;
    decision.statusIsError = true;
    decision.statusText = errorStatusText(reqId, error, payload, actionText);
    decision.handled = true;
    return decision;
}

} // namespace memochat::group
