#include "ChatHandlerRegistrars.h"
#include "ChatSessionService.h"
#include "ports/IGroupMessageService.h"
#include "ports/IPrivateMessageService.h"
#include "ports/IRelationService.h"

void ChatSessionServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[MSG_CHAT_LOGIN] = std::bind(&ChatSessionService::HandleLogin,
                                          logic._chat_session_service.get(),
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3);
    callbacks[ID_GET_RELATION_BOOTSTRAP_REQ] = std::bind(&ChatSessionService::HandleRelationBootstrap,
                                                         logic._chat_session_service.get(),
                                                         std::placeholders::_1,
                                                         std::placeholders::_2,
                                                         std::placeholders::_3);
    callbacks[ID_HEART_BEAT_REQ] = std::bind(&ChatSessionService::HandleHeartbeat,
                                             logic._chat_session_service.get(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
}

void ChatRelationServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_SEARCH_USER_REQ] = std::bind(&IRelationService::HandleSearchUser,
                                              logic._chat_relation_service.get(),
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3);
    callbacks[ID_ADD_FRIEND_REQ] = std::bind(&IRelationService::HandleAddFriendApply,
                                             logic._chat_relation_service.get(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&IRelationService::HandleAuthFriendApply,
                                              logic._chat_relation_service.get(),
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3);
    callbacks[ID_DELETE_FRIEND_REQ] = std::bind(&IRelationService::HandleDeleteFriend,
                                                logic._chat_relation_service.get(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3);
    callbacks[ID_GET_DIALOG_LIST_REQ] = std::bind(&IRelationService::HandleGetDialogList,
                                                  logic._chat_relation_service.get(),
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
    callbacks[ID_SYNC_DRAFT_REQ] = std::bind(&IRelationService::HandleSyncDraft,
                                             logic._chat_relation_service.get(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    callbacks[ID_PIN_DIALOG_REQ] = std::bind(&IRelationService::HandlePinDialog,
                                             logic._chat_relation_service.get(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
}

void PrivateMessageServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&IPrivateMessageService::HandleTextChatMessage,
                                                logic._private_message_service.get(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3);
    callbacks[ID_EDIT_PRIVATE_MSG_REQ] = std::bind(&IPrivateMessageService::HandleEditPrivateMessage,
                                                   logic._private_message_service.get(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    callbacks[ID_REVOKE_PRIVATE_MSG_REQ] = std::bind(&IPrivateMessageService::HandleRevokePrivateMessage,
                                                     logic._private_message_service.get(),
                                                     std::placeholders::_1,
                                                     std::placeholders::_2,
                                                     std::placeholders::_3);
    callbacks[ID_PRIVATE_HISTORY_REQ] = std::bind(&IPrivateMessageService::HandlePrivateHistory,
                                                  logic._private_message_service.get(),
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
    callbacks[ID_FORWARD_PRIVATE_MSG_REQ] = std::bind(&IPrivateMessageService::HandleForwardPrivateMessage,
                                                      logic._private_message_service.get(),
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3);
    callbacks[ID_PRIVATE_READ_ACK_REQ] = std::bind(&IPrivateMessageService::HandlePrivateReadAck,
                                                   logic._private_message_service.get(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
}

void GroupMessageServiceRegistrar::Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_CREATE_GROUP_REQ] = std::bind(&IGroupMessageService::HandleCreateGroup,
                                               logic._group_message_service.get(),
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3);
    callbacks[ID_GET_GROUP_LIST_REQ] = std::bind(&IGroupMessageService::HandleGetGroupList,
                                                 logic._group_message_service.get(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_INVITE_GROUP_MEMBER_REQ] = std::bind(&IGroupMessageService::HandleInviteGroupMember,
                                                      logic._group_message_service.get(),
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3);
    callbacks[ID_APPLY_JOIN_GROUP_REQ] = std::bind(&IGroupMessageService::HandleApplyJoinGroup,
                                                   logic._group_message_service.get(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    callbacks[ID_REVIEW_GROUP_APPLY_REQ] = std::bind(&IGroupMessageService::HandleReviewGroupApply,
                                                     logic._group_message_service.get(),
                                                     std::placeholders::_1,
                                                     std::placeholders::_2,
                                                     std::placeholders::_3);
    callbacks[ID_GROUP_CHAT_MSG_REQ] = std::bind(&IGroupMessageService::HandleGroupChatMessage,
                                                 logic._group_message_service.get(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_GROUP_HISTORY_REQ] = std::bind(&IGroupMessageService::HandleGroupHistory,
                                                logic._group_message_service.get(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3);
    callbacks[ID_EDIT_GROUP_MSG_REQ] = std::bind(&IGroupMessageService::HandleEditGroupMessage,
                                                 logic._group_message_service.get(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_REVOKE_GROUP_MSG_REQ] = std::bind(&IGroupMessageService::HandleRevokeGroupMessage,
                                                   logic._group_message_service.get(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    callbacks[ID_FORWARD_GROUP_MSG_REQ] = std::bind(&IGroupMessageService::HandleForwardGroupMessage,
                                                    logic._group_message_service.get(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_GROUP_READ_ACK_REQ] = std::bind(&IGroupMessageService::HandleGroupReadAck,
                                                 logic._group_message_service.get(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_UPDATE_GROUP_ANNOUNCEMENT_REQ] = std::bind(&IGroupMessageService::HandleUpdateGroupAnnouncement,
                                                            logic._group_message_service.get(),
                                                            std::placeholders::_1,
                                                            std::placeholders::_2,
                                                            std::placeholders::_3);
    callbacks[ID_UPDATE_GROUP_ICON_REQ] = std::bind(&IGroupMessageService::HandleUpdateGroupIcon,
                                                    logic._group_message_service.get(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_SET_GROUP_ADMIN_REQ] = std::bind(&IGroupMessageService::HandleSetGroupAdmin,
                                                  logic._group_message_service.get(),
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
    callbacks[ID_MUTE_GROUP_MEMBER_REQ] = std::bind(&IGroupMessageService::HandleMuteGroupMember,
                                                    logic._group_message_service.get(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_KICK_GROUP_MEMBER_REQ] = std::bind(&IGroupMessageService::HandleKickGroupMember,
                                                    logic._group_message_service.get(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_QUIT_GROUP_REQ] = std::bind(&IGroupMessageService::HandleQuitGroup,
                                             logic._group_message_service.get(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    callbacks[ID_DISSOLVE_GROUP_REQ] = std::bind(&IGroupMessageService::HandleDissolveGroup,
                                                 logic._group_message_service.get(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
}

void AsyncEventDispatcherRegistrar::Register(LogicSystem&, std::map<short, FunCallBack>&) const
{
}
