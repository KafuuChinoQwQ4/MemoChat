#include "ChatHandlerRegistrars.h"
#include "ChatRelationService.h"
#include "ChatSessionService.h"
#include "GroupMessageService.h"
#include "PrivateMessageService.h"

void ChatSessionServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[MSG_CHAT_LOGIN] = std::bind(&ChatSessionService::HandleLogin, logic._chat_session_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_GET_RELATION_BOOTSTRAP_REQ] = std::bind(&ChatSessionService::HandleRelationBootstrap, logic._chat_session_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_HEART_BEAT_REQ] = std::bind(&ChatSessionService::HandleHeartbeat, logic._chat_session_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void ChatRelationServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_SEARCH_USER_REQ] = std::bind(&ChatRelationService::HandleSearchUser, logic._chat_relation_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_ADD_FRIEND_REQ] = std::bind(&ChatRelationService::HandleAddFriendApply, logic._chat_relation_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&ChatRelationService::HandleAuthFriendApply, logic._chat_relation_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_GET_DIALOG_LIST_REQ] = std::bind(&ChatRelationService::HandleGetDialogList, logic._chat_relation_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_SYNC_DRAFT_REQ] = std::bind(&ChatRelationService::HandleSyncDraft, logic._chat_relation_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_PIN_DIALOG_REQ] = std::bind(&ChatRelationService::HandlePinDialog, logic._chat_relation_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void PrivateMessageServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&PrivateMessageService::HandleTextChatMessage, logic._private_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_EDIT_PRIVATE_MSG_REQ] = std::bind(&PrivateMessageService::HandleEditPrivateMessage, logic._private_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_REVOKE_PRIVATE_MSG_REQ] = std::bind(&PrivateMessageService::HandleRevokePrivateMessage, logic._private_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_PRIVATE_HISTORY_REQ] = std::bind(&PrivateMessageService::HandlePrivateHistory, logic._private_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_FORWARD_PRIVATE_MSG_REQ] = std::bind(&PrivateMessageService::HandleForwardPrivateMessage, logic._private_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_PRIVATE_READ_ACK_REQ] = std::bind(&PrivateMessageService::HandlePrivateReadAck, logic._private_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void GroupMessageServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_CREATE_GROUP_REQ] = std::bind(&GroupMessageService::HandleCreateGroup, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_GET_GROUP_LIST_REQ] = std::bind(&GroupMessageService::HandleGetGroupList, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_INVITE_GROUP_MEMBER_REQ] = std::bind(&GroupMessageService::HandleInviteGroupMember, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_APPLY_JOIN_GROUP_REQ] = std::bind(&GroupMessageService::HandleApplyJoinGroup, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_REVIEW_GROUP_APPLY_REQ] = std::bind(&GroupMessageService::HandleReviewGroupApply, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_GROUP_CHAT_MSG_REQ] = std::bind(&GroupMessageService::HandleGroupChatMessage, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_GROUP_HISTORY_REQ] = std::bind(&GroupMessageService::HandleGroupHistory, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_EDIT_GROUP_MSG_REQ] = std::bind(&GroupMessageService::HandleEditGroupMessage, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_REVOKE_GROUP_MSG_REQ] = std::bind(&GroupMessageService::HandleRevokeGroupMessage, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_FORWARD_GROUP_MSG_REQ] = std::bind(&GroupMessageService::HandleForwardGroupMessage, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_GROUP_READ_ACK_REQ] = std::bind(&GroupMessageService::HandleGroupReadAck, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_UPDATE_GROUP_ANNOUNCEMENT_REQ] = std::bind(&GroupMessageService::HandleUpdateGroupAnnouncement, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_UPDATE_GROUP_ICON_REQ] = std::bind(&GroupMessageService::HandleUpdateGroupIcon, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_SET_GROUP_ADMIN_REQ] = std::bind(&GroupMessageService::HandleSetGroupAdmin, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_MUTE_GROUP_MEMBER_REQ] = std::bind(&GroupMessageService::HandleMuteGroupMember, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_KICK_GROUP_MEMBER_REQ] = std::bind(&GroupMessageService::HandleKickGroupMember, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks[ID_QUIT_GROUP_REQ] = std::bind(&GroupMessageService::HandleQuitGroup, logic._group_message_service.get(),
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void AsyncEventDispatcherRegistrar::Register(LogicSystem&, std::map<short, FunCallBack>&) const
{
}
