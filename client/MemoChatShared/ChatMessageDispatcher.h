#ifndef CHATMESSAGEDISPATCHER_H
#define CHATMESSAGEDISPATCHER_H

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <functional>
#include <memory>

#include "global.h"
#include "userdata.h"

class ChatMessageDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit ChatMessageDispatcher(QObject *parent = nullptr);

    void dispatchMessage(ReqId id, int len, const QByteArray &data);

signals:
    void sig_swich_chatdlg();
    void sig_load_apply_list(QJsonArray json_array);
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);
    void sig_delete_friend_rsp(int error, int friendUid);
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
    void sig_group_list_updated();
    void sig_group_invite(qint64 groupId, QString groupCode, QString groupName, int operatorUid);
    void sig_group_apply(qint64 groupId, int applicantUid, QString applicantUserId, QString reason);
    void sig_group_member_changed(QJsonObject payload);
    void sig_group_chat_msg(std::shared_ptr<GroupChatMsg> msg);
    void sig_group_rsp(ReqId reqId, int error, QJsonObject payload);
    void sig_relation_bootstrap_updated();
    void sig_dialog_list_rsp(QJsonObject payload);
    void sig_private_history_rsp(QJsonObject payload);
    void sig_private_msg_changed(QJsonObject payload);
    void sig_private_read_ack(QJsonObject payload);
    void sig_message_status(QJsonObject payload);
    void sig_call_event(QJsonObject payload);
    void sig_heartbeat_ack(qint64 ackAtMs);
    void sig_notify_offline();

private:
    void initHandlers();

    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;
};

#endif // CHATMESSAGEDISPATCHER_H
