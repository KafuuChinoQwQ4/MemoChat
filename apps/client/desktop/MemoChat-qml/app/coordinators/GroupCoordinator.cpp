#include "AppCoordinators.h"

#include "AppController.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtGlobal>

GroupCoordinator::GroupCoordinator(AppController& controller)
    : _app(controller)
{
}

void GroupCoordinator::createGroup(const QString& name, const QVariantList& memberUserIdList)
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        _app.setGroupStatus("用户状态异常，请重新登录", true);
        return;
    }
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty() || trimmedName.size() > 64)
    {
        _app.setGroupStatus("群名称长度需在1-64之间", true);
        return;
    }

    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    QJsonArray memberUserIds;
    for (const auto& one : memberUserIdList)
    {
        const QString userId = one.toString().trimmed();
        if (userId.isEmpty())
        {
            continue;
        }
        if (!kUserIdPattern.match(userId).hasMatch())
        {
            _app.setGroupStatus(QString("成员ID格式非法：%1").arg(userId), true);
            return;
        }
        if (selfInfo->_user_id == userId)
        {
            continue;
        }
        memberUserIds.append(userId);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["name"] = trimmedName;
    obj["member_limit"] = 200;
    obj["member_user_ids"] = memberUserIds;
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_CREATE_GROUP_REQ,
                                                  QJsonDocument(obj).toJson(QJsonDocument::Compact));
    _app.setGroupStatus("正在创建群聊...", false);
}

void GroupCoordinator::inviteGroupMember(const QString& userId, const QString& reason)
{
    Q_UNUSED(reason);
    _app.inviteGroupMember(userId, reason);
}

void GroupCoordinator::applyJoinGroup(const QString& groupCode, const QString& reason)
{
    Q_UNUSED(reason);
    _app.applyJoinGroup(groupCode, reason);
}

void GroupCoordinator::reviewGroupApply(qint64 applyId, bool agree)
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || applyId <= 0)
    {
        _app.setGroupStatus("审核参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["apply_id"] = applyId;
    obj["agree"] = agree;
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_REVIEW_GROUP_APPLY_REQ,
                                                  QJsonDocument(obj).toJson(QJsonDocument::Compact));
    _app.setGroupStatus("审核请求已发送", false);
}

void GroupCoordinator::sendGroupTextMessage(const QString& text)
{
    if (_app._group_state.currentId <= 0)
    {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.sendTextMessage(text);
}

void GroupCoordinator::sendGroupImageMessage()
{
    if (_app._group_state.currentId <= 0)
    {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.sendImageMessage();
}

void GroupCoordinator::sendGroupFileMessage()
{
    if (_app._group_state.currentId <= 0)
    {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.sendFileMessage();
}

void GroupCoordinator::updateGroupAnnouncement(const QString& announcement)
{
    if (_app._group_state.currentId <= 0)
    {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.updateGroupAnnouncement(announcement);
}

void GroupCoordinator::setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits)
{
    _app.setGroupAdmin(userId, isAdmin, permissionBits);
}

void GroupCoordinator::muteGroupMember(const QString& userId, int muteSeconds)
{
    _app.muteGroupMember(userId, muteSeconds);
}

void GroupCoordinator::kickGroupMember(const QString& userId)
{
    _app.kickGroupMember(userId);
}

void GroupCoordinator::quitCurrentGroup()
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._group_state.currentId <= 0)
    {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_app._group_state.currentId);
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_QUIT_GROUP_REQ,
                                                  QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GroupCoordinator::dissolveCurrentGroup()
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._group_state.currentId <= 0)
    {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    if (_app.currentGroupRole() < 3)
    {
        _app.setGroupStatus("只有群主可以解散群聊", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_app._group_state.currentId);
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_DISSOLVE_GROUP_REQ,
                                                  QJsonDocument(obj).toJson(QJsonDocument::Compact));
    _app.setGroupStatus("正在解散群聊...", false);
}
