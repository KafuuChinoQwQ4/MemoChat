#include "AppChatDispatcherSignalRoutes.h"

#include "AppChatDispatcherEventRouter.h"
#include "ChatMessageDispatcher.h"

#include <QObject>

namespace memochat::app
{
void bindAppChatDispatcherSignalRoutes(const std::shared_ptr<ChatMessageDispatcher>& chatDispatcher,
                                       AppChatDispatcherEventRouter& router)
{
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_swich_chatdlg,
                     &router,
                     &AppChatDispatcherEventRouter::onSwitchToChat);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_add_auth_friend,
                     &router,
                     &AppChatDispatcherEventRouter::onAddAuthFriend);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_auth_rsp,
                     &router,
                     &AppChatDispatcherEventRouter::onAuthRsp);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_delete_friend_rsp,
                     &router,
                     &AppChatDispatcherEventRouter::onDeleteFriendRsp);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_text_chat_msg,
                     &router,
                     &AppChatDispatcherEventRouter::onTextChatMsg);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_user_search,
                     &router,
                     &AppChatDispatcherEventRouter::onUserSearch);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_friend_apply,
                     &router,
                     &AppChatDispatcherEventRouter::onFriendApply);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_group_list_updated,
                     &router,
                     &AppChatDispatcherEventRouter::onGroupListUpdated);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_group_invite,
                     &router,
                     &AppChatDispatcherEventRouter::onGroupInvite);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_group_apply,
                     &router,
                     &AppChatDispatcherEventRouter::onGroupApply);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_group_member_changed,
                     &router,
                     &AppChatDispatcherEventRouter::onGroupMemberChanged);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_group_chat_msg,
                     &router,
                     &AppChatDispatcherEventRouter::onGroupChatMsg);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_group_rsp,
                     &router,
                     &AppChatDispatcherEventRouter::onGroupRsp);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_relation_bootstrap_updated,
                     &router,
                     &AppChatDispatcherEventRouter::onRelationBootstrapUpdated);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_dialog_list_rsp,
                     &router,
                     &AppChatDispatcherEventRouter::onDialogListRsp);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_private_history_rsp,
                     &router,
                     &AppChatDispatcherEventRouter::onPrivateHistoryRsp);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_private_msg_changed,
                     &router,
                     &AppChatDispatcherEventRouter::onPrivateMsgChanged);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_private_read_ack,
                     &router,
                     &AppChatDispatcherEventRouter::onPrivateReadAck);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_message_status,
                     &router,
                     &AppChatDispatcherEventRouter::onMessageStatus);
    QObject::connect(chatDispatcher.get(),
                     &ChatMessageDispatcher::sig_call_event,
                     &router,
                     &AppChatDispatcherEventRouter::onCallEvent);
}
} // namespace memochat::app
