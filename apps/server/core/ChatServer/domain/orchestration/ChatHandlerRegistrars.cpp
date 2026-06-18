#include "ChatHandlerRegistrars.h"
#include "ChatRuntimeComposition.h"
#include "ChatSessionService.h"
#include "const.h"
#include "ports/IGroupMessageService.h"
#include "ports/IPrivateMessageService.h"
#include "ports/IRelationSessionService.h"

void ChatSessionServiceRegistrar::Register(ChatRuntimeComposition& composition,
                                           std::map<short, FunCallBack>& callbacks) const
{
    callbacks[MSG_CHAT_LOGIN] = std::bind(&ChatSessionService::HandleLogin,
                                          &composition.SessionService(),
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3);
    callbacks[ID_GET_RELATION_BOOTSTRAP_REQ] = std::bind(&ChatSessionService::HandleRelationBootstrap,
                                                         &composition.SessionService(),
                                                         std::placeholders::_1,
                                                         std::placeholders::_2,
                                                         std::placeholders::_3);
    callbacks[ID_HEART_BEAT_REQ] = std::bind(&ChatSessionService::HandleHeartbeat,
                                             &composition.SessionService(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
}

void ChatRelationServiceRegistrar::Register(ChatRuntimeComposition& composition,
                                            std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_SEARCH_USER_REQ] = std::bind(&IRelationSessionService::HandleSearchUser,
                                              &composition.RelationSessionService(),
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3);
    callbacks[ID_ADD_FRIEND_REQ] = std::bind(&IRelationSessionService::HandleAddFriendApply,
                                             &composition.RelationSessionService(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&IRelationSessionService::HandleAuthFriendApply,
                                              &composition.RelationSessionService(),
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3);
    callbacks[ID_DELETE_FRIEND_REQ] = std::bind(&IRelationSessionService::HandleDeleteFriend,
                                                &composition.RelationSessionService(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3);
    callbacks[ID_GET_DIALOG_LIST_REQ] = std::bind(&IRelationSessionService::HandleGetDialogList,
                                                  &composition.RelationSessionService(),
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
    callbacks[ID_SYNC_DRAFT_REQ] = std::bind(&IRelationSessionService::HandleSyncDraft,
                                             &composition.RelationSessionService(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    callbacks[ID_PIN_DIALOG_REQ] = std::bind(&IRelationSessionService::HandlePinDialog,
                                             &composition.RelationSessionService(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
}

void PrivateMessageServiceRegistrar::Register(ChatRuntimeComposition& composition,
                                              std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&IPrivateMessageService::HandleTextChatMessage,
                                                &composition.PrivateMessageService(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3);
    callbacks[ID_EDIT_PRIVATE_MSG_REQ] = std::bind(&IPrivateMessageService::HandleEditPrivateMessage,
                                                   &composition.PrivateMessageService(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    callbacks[ID_REVOKE_PRIVATE_MSG_REQ] = std::bind(&IPrivateMessageService::HandleRevokePrivateMessage,
                                                     &composition.PrivateMessageService(),
                                                     std::placeholders::_1,
                                                     std::placeholders::_2,
                                                     std::placeholders::_3);
    callbacks[ID_PRIVATE_HISTORY_REQ] = std::bind(&IPrivateMessageService::HandlePrivateHistory,
                                                  &composition.PrivateMessageService(),
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
    callbacks[ID_FORWARD_PRIVATE_MSG_REQ] = std::bind(&IPrivateMessageService::HandleForwardPrivateMessage,
                                                      &composition.PrivateMessageService(),
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3);
    callbacks[ID_PRIVATE_READ_ACK_REQ] = std::bind(&IPrivateMessageService::HandlePrivateReadAck,
                                                   &composition.PrivateMessageService(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
}

void GroupMessageServiceRegistrar::Register(ChatRuntimeComposition& composition,
                                            std::map<short, FunCallBack>& callbacks) const
{
    callbacks[ID_CREATE_GROUP_REQ] = std::bind(&IGroupMessageService::HandleCreateGroup,
                                               &composition.GroupMessageService(),
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3);
    callbacks[ID_GET_GROUP_LIST_REQ] = std::bind(&IGroupMessageService::HandleGetGroupList,
                                                 &composition.GroupMessageService(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_INVITE_GROUP_MEMBER_REQ] = std::bind(&IGroupMessageService::HandleInviteGroupMember,
                                                      &composition.GroupMessageService(),
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3);
    callbacks[ID_APPLY_JOIN_GROUP_REQ] = std::bind(&IGroupMessageService::HandleApplyJoinGroup,
                                                   &composition.GroupMessageService(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    callbacks[ID_REVIEW_GROUP_APPLY_REQ] = std::bind(&IGroupMessageService::HandleReviewGroupApply,
                                                     &composition.GroupMessageService(),
                                                     std::placeholders::_1,
                                                     std::placeholders::_2,
                                                     std::placeholders::_3);
    callbacks[ID_GROUP_CHAT_MSG_REQ] = std::bind(&IGroupMessageService::HandleGroupChatMessage,
                                                 &composition.GroupMessageService(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_GROUP_HISTORY_REQ] = std::bind(&IGroupMessageService::HandleGroupHistory,
                                                &composition.GroupMessageService(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3);
    callbacks[ID_EDIT_GROUP_MSG_REQ] = std::bind(&IGroupMessageService::HandleEditGroupMessage,
                                                 &composition.GroupMessageService(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_REVOKE_GROUP_MSG_REQ] = std::bind(&IGroupMessageService::HandleRevokeGroupMessage,
                                                   &composition.GroupMessageService(),
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3);
    callbacks[ID_FORWARD_GROUP_MSG_REQ] = std::bind(&IGroupMessageService::HandleForwardGroupMessage,
                                                    &composition.GroupMessageService(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_GROUP_READ_ACK_REQ] = std::bind(&IGroupMessageService::HandleGroupReadAck,
                                                 &composition.GroupMessageService(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
    callbacks[ID_UPDATE_GROUP_ANNOUNCEMENT_REQ] = std::bind(&IGroupMessageService::HandleUpdateGroupAnnouncement,
                                                            &composition.GroupMessageService(),
                                                            std::placeholders::_1,
                                                            std::placeholders::_2,
                                                            std::placeholders::_3);
    callbacks[ID_UPDATE_GROUP_ICON_REQ] = std::bind(&IGroupMessageService::HandleUpdateGroupIcon,
                                                    &composition.GroupMessageService(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_SET_GROUP_ADMIN_REQ] = std::bind(&IGroupMessageService::HandleSetGroupAdmin,
                                                  &composition.GroupMessageService(),
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
    callbacks[ID_MUTE_GROUP_MEMBER_REQ] = std::bind(&IGroupMessageService::HandleMuteGroupMember,
                                                    &composition.GroupMessageService(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_KICK_GROUP_MEMBER_REQ] = std::bind(&IGroupMessageService::HandleKickGroupMember,
                                                    &composition.GroupMessageService(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3);
    callbacks[ID_QUIT_GROUP_REQ] = std::bind(&IGroupMessageService::HandleQuitGroup,
                                             &composition.GroupMessageService(),
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    callbacks[ID_DISSOLVE_GROUP_REQ] = std::bind(&IGroupMessageService::HandleDissolveGroup,
                                                 &composition.GroupMessageService(),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
}

void AsyncEventDispatcherRegistrar::Register(ChatRuntimeComposition&, std::map<short, FunCallBack>&) const
{
}
