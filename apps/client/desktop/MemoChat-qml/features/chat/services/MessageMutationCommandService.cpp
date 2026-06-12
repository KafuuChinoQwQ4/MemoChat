#include "MessageMutationCommandService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace
{
constexpr int kMaxMessageContentLength = 4096;
constexpr int kEditGroupMsgReq = 1065;
constexpr int kRevokeGroupMsgReq = 1067;
constexpr int kForwardGroupMsgReq = 1069;
constexpr int kEditPrivateMsgReq = 1077;
constexpr int kRevokePrivateMsgReq = 1079;
constexpr int kForwardPrivateMsgReq = 1083;

QByteArray compactJson(const QJsonObject& payload)
{
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QString invalidTextFor(MessageMutationCommand command)
{
    switch (command)
    {
        case MessageMutationCommand::Edit:
            return QStringLiteral("编辑消息参数非法");
        case MessageMutationCommand::Revoke:
            return QStringLiteral("撤回消息参数非法");
        case MessageMutationCommand::Forward:
            return QStringLiteral("转发消息参数非法");
    }
    return QStringLiteral("消息参数非法");
}

void applyStatus(const MessageMutationCommandDependencies& dependencies,
                 bool groupStatus,
                 const QString& text,
                 bool isError)
{
    if (groupStatus && dependencies.setGroupStatus)
    {
        dependencies.setGroupStatus(text, isError);
        return;
    }
    if (dependencies.setPrivateStatus)
    {
        dependencies.setPrivateStatus(text, isError);
    }
}

MessageMutationCommandResult failed(MessageMutationCommand command,
                                    const MessageMutationCommandDependencies& dependencies,
                                    bool groupStatus,
                                    const QString& errorText)
{
    MessageMutationCommandResult result;
    result.errorText = errorText.isEmpty() ? invalidTextFor(command) : errorText;
    result.errorIsGroup = groupStatus;
    applyStatus(dependencies, groupStatus, result.errorText, true);
    return result;
}
} // namespace

QByteArray MessageMutationCommandService::buildPayload(const MessageMutationCommandRequest& request)
{
    QJsonObject payload;
    payload[QStringLiteral("fromuid")] = request.selfUid;
    payload[QStringLiteral("msgid")] = request.msgId.trimmed();
    if (request.command == MessageMutationCommand::Edit)
    {
        payload[QStringLiteral("content")] = request.text.trimmed().left(kMaxMessageContentLength);
    }
    if (request.command == MessageMutationCommand::Forward)
    {
        payload[QStringLiteral("client_msg_id")] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (request.target.isGroup())
    {
        payload[QStringLiteral("groupid")] = static_cast<qint64>(request.target.groupId);
    }
    else
    {
        payload[QStringLiteral("peer_uid")] = request.target.peerUid;
    }
    return compactJson(payload);
}

int MessageMutationCommandService::requestIdFor(MessageMutationCommand command, const MessageMutationTarget& target)
{
    switch (command)
    {
        case MessageMutationCommand::Edit:
            return target.isGroup() ? kEditGroupMsgReq : kEditPrivateMsgReq;
        case MessageMutationCommand::Revoke:
            return target.isGroup() ? kRevokeGroupMsgReq : kRevokePrivateMsgReq;
        case MessageMutationCommand::Forward:
            return target.isGroup() ? kForwardGroupMsgReq : kForwardPrivateMsgReq;
    }
    return 0;
}

MessageMutationCommandResult MessageMutationCommandService::run(const MessageMutationCommandRequest& request,
                                                                const MessageMutationCommandDependencies& dependencies)
{
    const QString msgId = request.msgId.trimmed();
    const QString text = request.text.trimmed();
    const bool useGroupStatus = request.target.isGroup();
    if (request.selfUid <= 0 || msgId.isEmpty() || (request.command == MessageMutationCommand::Edit && text.isEmpty()))
    {
        return failed(request.command, dependencies, useGroupStatus, QString());
    }
    if (!request.target.isGroup() && !request.target.isPrivate())
    {
        return failed(request.command, dependencies, false, QStringLiteral("请选择会话"));
    }
    if (request.command == MessageMutationCommand::Forward && request.target.isPrivate())
    {
        if (!dependencies.privateMessageExists || !dependencies.privateMessageExists(msgId))
        {
            return failed(request.command, dependencies, false, QStringLiteral("未找到要转发的消息"));
        }
    }
    if (request.command == MessageMutationCommand::Revoke &&
        (!dependencies.canRevokeMessage || !dependencies.canRevokeMessage(msgId)))
    {
        return failed(request.command, dependencies, useGroupStatus, QStringLiteral("只能撤回5分钟内自己发送的消息"));
    }
    if (!dependencies.dispatchPayload)
    {
        return failed(request.command, dependencies, useGroupStatus, QStringLiteral("消息发送器不可用"));
    }

    MessageMutationCommandRequest normalized = request;
    normalized.msgId = msgId;
    normalized.text = text;

    MessageMutationCommandResult result;
    result.reqId = requestIdFor(normalized.command, normalized.target);
    result.compactPayload = buildPayload(normalized);
    dependencies.dispatchPayload(result.reqId, result.compactPayload);
    result.dispatched = true;
    result.success = true;
    return result;
}
