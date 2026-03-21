/****************************************************************************
** Meta object code from reading C++ file 'AppController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../../client/MemoChat-qml/AppController.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AppController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13AppControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13AppControllerE = QtMocHelpers::stringData(
    "AppController",
    "pageChanged",
    "",
    "tipChanged",
    "busyChanged",
    "registerSuccessPageChanged",
    "registerCountdownChanged",
    "chatTabChanged",
    "contactPaneChanged",
    "currentContactChanged",
    "currentUserChanged",
    "currentChatPeerChanged",
    "currentDialogUidChanged",
    "searchPendingChanged",
    "searchStatusChanged",
    "pendingApplyChanged",
    "chatLoadingMoreChanged",
    "canLoadMoreChatsChanged",
    "privateHistoryLoadingChanged",
    "canLoadMorePrivateHistoryChanged",
    "contactLoadingMoreChanged",
    "canLoadMoreContactsChanged",
    "authStatusChanged",
    "settingsStatusChanged",
    "currentGroupChanged",
    "groupStatusChanged",
    "mediaUploadStateChanged",
    "currentDraftTextChanged",
    "currentPendingAttachmentsChanged",
    "currentDialogPinnedChanged",
    "currentDialogMutedChanged",
    "pendingReplyChanged",
    "lazyBootstrapStateChanged",
    "onLoginHttpFinished",
    "ReqId",
    "id",
    "res",
    "ErrorCodes",
    "err",
    "onRegisterHttpFinished",
    "onResetHttpFinished",
    "onSettingsHttpFinished",
    "onCallHttpFinished",
    "onTcpConnectFinished",
    "success",
    "onChatLoginFailed",
    "onSwitchToChat",
    "onRelationBootstrapUpdated",
    "onRegisterCountdownTimeout",
    "onHeartbeatTimeout",
    "onHeartbeatAck",
    "ackAtMs",
    "onAddAuthFriend",
    "std::shared_ptr<AuthInfo>",
    "authInfo",
    "onAuthRsp",
    "std::shared_ptr<AuthRsp>",
    "authRsp",
    "onTextChatMsg",
    "std::shared_ptr<TextChatMsg>",
    "msg",
    "onUserSearch",
    "std::shared_ptr<SearchInfo>",
    "searchInfo",
    "onFriendApply",
    "std::shared_ptr<AddFriendApply>",
    "applyInfo",
    "onNotifyOffline",
    "onConnectionClosed",
    "onGroupListUpdated",
    "onGroupInvite",
    "groupId",
    "groupCode",
    "groupName",
    "operatorUid",
    "onGroupApply",
    "applicantUid",
    "applicantUserId",
    "reason",
    "onGroupMemberChanged",
    "payload",
    "onGroupChatMsg",
    "std::shared_ptr<GroupChatMsg>",
    "onGroupRsp",
    "reqId",
    "error",
    "onDialogListRsp",
    "onPrivateHistoryRsp",
    "onPrivateMsgChanged",
    "onPrivateReadAck",
    "onMessageStatus",
    "onCallEvent",
    "onLivekitRoomJoined",
    "onLivekitRemoteTrackReady",
    "onLivekitRoomDisconnected",
    "recoverable",
    "onLivekitPermissionError",
    "deviceType",
    "message",
    "onLivekitMediaError",
    "onLivekitReconnecting",
    "switchToLogin",
    "switchToRegister",
    "switchToReset",
    "switchChatTab",
    "tab",
    "ensureContactsInitialized",
    "ensureGroupsInitialized",
    "ensureApplyInitialized",
    "ensureChatListInitialized",
    "clearTip",
    "selectChatIndex",
    "index",
    "selectGroupIndex",
    "selectDialogByUid",
    "uid",
    "selectContactIndex",
    "showApplyRequests",
    "jumpChatWithCurrentContact",
    "loadMoreChats",
    "loadMorePrivateHistory",
    "loadMoreContacts",
    "sendTextMessage",
    "text",
    "sendCurrentComposerPayload",
    "sendImageMessage",
    "sendFileMessage",
    "removePendingAttachment",
    "attachmentId",
    "clearPendingAttachments",
    "openExternalResource",
    "url",
    "searchUser",
    "uidText",
    "clearSearchState",
    "requestAddFriend",
    "bakName",
    "QVariantList",
    "labels",
    "approveFriend",
    "backName",
    "clearAuthStatus",
    "startVoiceChat",
    "startVideoChat",
    "acceptIncomingCall",
    "rejectIncomingCall",
    "endCurrentCall",
    "toggleCallMuted",
    "toggleCallCamera",
    "chooseAvatar",
    "saveProfile",
    "nick",
    "desc",
    "clearSettingsStatus",
    "refreshGroupList",
    "createGroup",
    "name",
    "memberUserIdList",
    "inviteGroupMember",
    "userId",
    "applyJoinGroup",
    "reviewGroupApply",
    "applyId",
    "agree",
    "sendGroupTextMessage",
    "sendGroupImageMessage",
    "sendGroupFileMessage",
    "editGroupMessage",
    "msgId",
    "revokeGroupMessage",
    "forwardGroupMessage",
    "loadGroupHistory",
    "updateGroupAnnouncement",
    "announcement",
    "updateGroupIcon",
    "setGroupAdmin",
    "isAdmin",
    "permissionBits",
    "muteGroupMember",
    "muteSeconds",
    "kickGroupMember",
    "quitCurrentGroup",
    "clearGroupStatus",
    "updateCurrentDraft",
    "toggleCurrentDialogPinned",
    "toggleCurrentDialogMuted",
    "toggleDialogPinnedByUid",
    "dialogUid",
    "toggleDialogMutedByUid",
    "markDialogReadByUid",
    "clearDialogDraftByUid",
    "beginReplyMessage",
    "senderName",
    "previewText",
    "cancelReplyMessage",
    "login",
    "email",
    "password",
    "requestRegisterCode",
    "registerUser",
    "user",
    "confirm",
    "verifyCode",
    "requestResetCode",
    "resetPassword",
    "page",
    "Page",
    "tipText",
    "tipError",
    "busy",
    "registerSuccessPage",
    "registerCountdown",
    "chatTab",
    "ChatTab",
    "currentUserName",
    "currentUserNick",
    "currentUserIcon",
    "currentUserDesc",
    "currentUserId",
    "contactPane",
    "ContactPane",
    "currentContactName",
    "currentContactNick",
    "currentContactIcon",
    "currentContactBack",
    "currentContactSex",
    "currentContactUserId",
    "hasCurrentContact",
    "currentChatPeerName",
    "currentChatPeerIcon",
    "hasCurrentChat",
    "currentDialogUid",
    "hasCurrentGroup",
    "currentGroupRole",
    "currentGroupName",
    "currentGroupCode",
    "currentGroupCanChangeInfo",
    "currentGroupCanInviteUsers",
    "currentGroupCanManageAdmins",
    "currentGroupCanBanUsers",
    "dialogListModel",
    "FriendListModel*",
    "chatListModel",
    "groupListModel",
    "contactListModel",
    "messageModel",
    "ChatMessageModel*",
    "searchResultModel",
    "SearchResultModel*",
    "applyRequestModel",
    "ApplyRequestModel*",
    "searchPending",
    "searchStatusText",
    "searchStatusError",
    "hasPendingApply",
    "chatLoadingMore",
    "canLoadMoreChats",
    "privateHistoryLoading",
    "canLoadMorePrivateHistory",
    "contactLoadingMore",
    "canLoadMoreContacts",
    "authStatusText",
    "authStatusError",
    "settingsStatusText",
    "settingsStatusError",
    "groupStatusText",
    "groupStatusError",
    "mediaUploadInProgress",
    "mediaUploadProgressText",
    "callSession",
    "CallSessionModel*",
    "livekitBridge",
    "LivekitBridge*",
    "currentDraftText",
    "currentPendingAttachments",
    "hasPendingAttachments",
    "currentDialogPinned",
    "currentDialogMuted",
    "hasPendingReply",
    "replyTargetName",
    "replyPreviewText",
    "dialogsReady",
    "contactsReady",
    "groupsReady",
    "applyReady",
    "chatShellBusy",
    "LoginPage",
    "RegisterPage",
    "ResetPage",
    "ChatPage",
    "ChatTabPage",
    "ContactTabPage",
    "SettingsTabPage",
    "ApplyRequestPane",
    "FriendInfoPane"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13AppControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
     147,   14, // methods
      72, 1275, // properties
       3, 1635, // enums/sets
       0,    0, // constructors
       0,       // flags
      31,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  896,    2, 0x06,   76 /* Public */,
       3,    0,  897,    2, 0x06,   77 /* Public */,
       4,    0,  898,    2, 0x06,   78 /* Public */,
       5,    0,  899,    2, 0x06,   79 /* Public */,
       6,    0,  900,    2, 0x06,   80 /* Public */,
       7,    0,  901,    2, 0x06,   81 /* Public */,
       8,    0,  902,    2, 0x06,   82 /* Public */,
       9,    0,  903,    2, 0x06,   83 /* Public */,
      10,    0,  904,    2, 0x06,   84 /* Public */,
      11,    0,  905,    2, 0x06,   85 /* Public */,
      12,    0,  906,    2, 0x06,   86 /* Public */,
      13,    0,  907,    2, 0x06,   87 /* Public */,
      14,    0,  908,    2, 0x06,   88 /* Public */,
      15,    0,  909,    2, 0x06,   89 /* Public */,
      16,    0,  910,    2, 0x06,   90 /* Public */,
      17,    0,  911,    2, 0x06,   91 /* Public */,
      18,    0,  912,    2, 0x06,   92 /* Public */,
      19,    0,  913,    2, 0x06,   93 /* Public */,
      20,    0,  914,    2, 0x06,   94 /* Public */,
      21,    0,  915,    2, 0x06,   95 /* Public */,
      22,    0,  916,    2, 0x06,   96 /* Public */,
      23,    0,  917,    2, 0x06,   97 /* Public */,
      24,    0,  918,    2, 0x06,   98 /* Public */,
      25,    0,  919,    2, 0x06,   99 /* Public */,
      26,    0,  920,    2, 0x06,  100 /* Public */,
      27,    0,  921,    2, 0x06,  101 /* Public */,
      28,    0,  922,    2, 0x06,  102 /* Public */,
      29,    0,  923,    2, 0x06,  103 /* Public */,
      30,    0,  924,    2, 0x06,  104 /* Public */,
      31,    0,  925,    2, 0x06,  105 /* Public */,
      32,    0,  926,    2, 0x06,  106 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      33,    3,  927,    2, 0x08,  107 /* Private */,
      39,    3,  934,    2, 0x08,  111 /* Private */,
      40,    3,  941,    2, 0x08,  115 /* Private */,
      41,    3,  948,    2, 0x08,  119 /* Private */,
      42,    3,  955,    2, 0x08,  123 /* Private */,
      43,    1,  962,    2, 0x08,  127 /* Private */,
      45,    1,  965,    2, 0x08,  129 /* Private */,
      46,    0,  968,    2, 0x08,  131 /* Private */,
      47,    0,  969,    2, 0x08,  132 /* Private */,
      48,    0,  970,    2, 0x08,  133 /* Private */,
      49,    0,  971,    2, 0x08,  134 /* Private */,
      50,    1,  972,    2, 0x08,  135 /* Private */,
      52,    1,  975,    2, 0x08,  137 /* Private */,
      55,    1,  978,    2, 0x08,  139 /* Private */,
      58,    1,  981,    2, 0x08,  141 /* Private */,
      61,    1,  984,    2, 0x08,  143 /* Private */,
      64,    1,  987,    2, 0x08,  145 /* Private */,
      67,    0,  990,    2, 0x08,  147 /* Private */,
      68,    0,  991,    2, 0x08,  148 /* Private */,
      69,    0,  992,    2, 0x08,  149 /* Private */,
      70,    4,  993,    2, 0x08,  150 /* Private */,
      75,    4, 1002,    2, 0x08,  155 /* Private */,
      79,    1, 1011,    2, 0x08,  160 /* Private */,
      81,    1, 1014,    2, 0x08,  162 /* Private */,
      83,    3, 1017,    2, 0x08,  164 /* Private */,
      86,    1, 1024,    2, 0x08,  168 /* Private */,
      87,    1, 1027,    2, 0x08,  170 /* Private */,
      88,    1, 1030,    2, 0x08,  172 /* Private */,
      89,    1, 1033,    2, 0x08,  174 /* Private */,
      90,    1, 1036,    2, 0x08,  176 /* Private */,
      91,    1, 1039,    2, 0x08,  178 /* Private */,
      92,    0, 1042,    2, 0x08,  180 /* Private */,
      93,    0, 1043,    2, 0x08,  181 /* Private */,
      94,    2, 1044,    2, 0x08,  182 /* Private */,
      96,    2, 1049,    2, 0x08,  185 /* Private */,
      99,    1, 1054,    2, 0x08,  188 /* Private */,
     100,    1, 1057,    2, 0x08,  190 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
     101,    0, 1060,    2, 0x02,  192 /* Public */,
     102,    0, 1061,    2, 0x02,  193 /* Public */,
     103,    0, 1062,    2, 0x02,  194 /* Public */,
     104,    1, 1063,    2, 0x02,  195 /* Public */,
     106,    0, 1066,    2, 0x02,  197 /* Public */,
     107,    0, 1067,    2, 0x02,  198 /* Public */,
     108,    0, 1068,    2, 0x02,  199 /* Public */,
     109,    0, 1069,    2, 0x02,  200 /* Public */,
     110,    0, 1070,    2, 0x02,  201 /* Public */,
     111,    1, 1071,    2, 0x02,  202 /* Public */,
     113,    1, 1074,    2, 0x02,  204 /* Public */,
     114,    1, 1077,    2, 0x02,  206 /* Public */,
     116,    1, 1080,    2, 0x02,  208 /* Public */,
     117,    0, 1083,    2, 0x02,  210 /* Public */,
     118,    0, 1084,    2, 0x02,  211 /* Public */,
     119,    0, 1085,    2, 0x02,  212 /* Public */,
     120,    0, 1086,    2, 0x02,  213 /* Public */,
     121,    0, 1087,    2, 0x02,  214 /* Public */,
     122,    1, 1088,    2, 0x02,  215 /* Public */,
     124,    1, 1091,    2, 0x02,  217 /* Public */,
     125,    0, 1094,    2, 0x02,  219 /* Public */,
     126,    0, 1095,    2, 0x02,  220 /* Public */,
     127,    1, 1096,    2, 0x02,  221 /* Public */,
     129,    0, 1099,    2, 0x02,  223 /* Public */,
     130,    1, 1100,    2, 0x02,  224 /* Public */,
     132,    1, 1103,    2, 0x02,  226 /* Public */,
     134,    0, 1106,    2, 0x02,  228 /* Public */,
     135,    3, 1107,    2, 0x02,  229 /* Public */,
     135,    2, 1114,    2, 0x22,  233 /* Public | MethodCloned */,
     139,    3, 1119,    2, 0x02,  236 /* Public */,
     139,    2, 1126,    2, 0x22,  240 /* Public | MethodCloned */,
     141,    0, 1131,    2, 0x02,  243 /* Public */,
     142,    0, 1132,    2, 0x02,  244 /* Public */,
     143,    0, 1133,    2, 0x02,  245 /* Public */,
     144,    0, 1134,    2, 0x02,  246 /* Public */,
     145,    0, 1135,    2, 0x02,  247 /* Public */,
     146,    0, 1136,    2, 0x02,  248 /* Public */,
     147,    0, 1137,    2, 0x02,  249 /* Public */,
     148,    0, 1138,    2, 0x02,  250 /* Public */,
     149,    0, 1139,    2, 0x02,  251 /* Public */,
     150,    2, 1140,    2, 0x02,  252 /* Public */,
     153,    0, 1145,    2, 0x02,  255 /* Public */,
     154,    0, 1146,    2, 0x02,  256 /* Public */,
     155,    2, 1147,    2, 0x02,  257 /* Public */,
     155,    1, 1152,    2, 0x22,  260 /* Public | MethodCloned */,
     158,    2, 1155,    2, 0x02,  262 /* Public */,
     158,    1, 1160,    2, 0x22,  265 /* Public | MethodCloned */,
     160,    2, 1163,    2, 0x02,  267 /* Public */,
     160,    1, 1168,    2, 0x22,  270 /* Public | MethodCloned */,
     161,    2, 1171,    2, 0x02,  272 /* Public */,
     164,    1, 1176,    2, 0x02,  275 /* Public */,
     165,    0, 1179,    2, 0x02,  277 /* Public */,
     166,    0, 1180,    2, 0x02,  278 /* Public */,
     167,    2, 1181,    2, 0x02,  279 /* Public */,
     169,    1, 1186,    2, 0x02,  282 /* Public */,
     170,    1, 1189,    2, 0x02,  284 /* Public */,
     171,    0, 1192,    2, 0x02,  286 /* Public */,
     172,    1, 1193,    2, 0x02,  287 /* Public */,
     174,    0, 1196,    2, 0x02,  289 /* Public */,
     175,    3, 1197,    2, 0x02,  290 /* Public */,
     175,    2, 1204,    2, 0x22,  294 /* Public | MethodCloned */,
     178,    2, 1209,    2, 0x02,  297 /* Public */,
     180,    1, 1214,    2, 0x02,  300 /* Public */,
     181,    0, 1217,    2, 0x02,  302 /* Public */,
     182,    0, 1218,    2, 0x02,  303 /* Public */,
     183,    1, 1219,    2, 0x02,  304 /* Public */,
     184,    0, 1222,    2, 0x02,  306 /* Public */,
     185,    0, 1223,    2, 0x02,  307 /* Public */,
     186,    1, 1224,    2, 0x02,  308 /* Public */,
     188,    1, 1227,    2, 0x02,  310 /* Public */,
     189,    1, 1230,    2, 0x02,  312 /* Public */,
     190,    1, 1233,    2, 0x02,  314 /* Public */,
     191,    3, 1236,    2, 0x02,  316 /* Public */,
     194,    0, 1243,    2, 0x02,  320 /* Public */,
     195,    2, 1244,    2, 0x02,  321 /* Public */,
     198,    1, 1249,    2, 0x02,  324 /* Public */,
     199,    5, 1252,    2, 0x02,  326 /* Public */,
     203,    1, 1263,    2, 0x02,  332 /* Public */,
     204,    4, 1266,    2, 0x02,  334 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 34, QMetaType::QString, 0x80000000 | 37,   35,   36,   38,
    QMetaType::Void, 0x80000000 | 34, QMetaType::QString, 0x80000000 | 37,   35,   36,   38,
    QMetaType::Void, 0x80000000 | 34, QMetaType::QString, 0x80000000 | 37,   35,   36,   38,
    QMetaType::Void, 0x80000000 | 34, QMetaType::QString, 0x80000000 | 37,   35,   36,   38,
    QMetaType::Void, 0x80000000 | 34, QMetaType::QString, 0x80000000 | 37,   35,   36,   38,
    QMetaType::Void, QMetaType::Bool,   44,
    QMetaType::Void, QMetaType::Int,   38,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,   51,
    QMetaType::Void, 0x80000000 | 53,   54,
    QMetaType::Void, 0x80000000 | 56,   57,
    QMetaType::Void, 0x80000000 | 59,   60,
    QMetaType::Void, 0x80000000 | 62,   63,
    QMetaType::Void, 0x80000000 | 65,   66,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::Int,   71,   72,   73,   74,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Int, QMetaType::QString, QMetaType::QString,   71,   76,   77,   78,
    QMetaType::Void, QMetaType::QJsonObject,   80,
    QMetaType::Void, 0x80000000 | 82,   60,
    QMetaType::Void, 0x80000000 | 34, QMetaType::Int, QMetaType::QJsonObject,   84,   85,   80,
    QMetaType::Void, QMetaType::QJsonObject,   80,
    QMetaType::Void, QMetaType::QJsonObject,   80,
    QMetaType::Void, QMetaType::QJsonObject,   80,
    QMetaType::Void, QMetaType::QJsonObject,   80,
    QMetaType::Void, QMetaType::QJsonObject,   80,
    QMetaType::Void, QMetaType::QJsonObject,   80,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   78,   95,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   97,   98,
    QMetaType::Void, QMetaType::QString,   98,
    QMetaType::Void, QMetaType::QString,   98,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,  105,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,  112,
    QMetaType::Void, QMetaType::Int,  112,
    QMetaType::Void, QMetaType::Int,  115,
    QMetaType::Void, QMetaType::Int,  112,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,  123,
    QMetaType::Void, QMetaType::QString,  123,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,  128,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,  131,
    QMetaType::Void, QMetaType::QString,  133,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, 0x80000000 | 137,  115,  136,  138,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,  115,  136,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, 0x80000000 | 137,  115,  140,  138,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,  115,  140,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,  151,  152,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 137,  156,  157,
    QMetaType::Void, QMetaType::QString,  156,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,  159,   78,
    QMetaType::Void, QMetaType::QString,  159,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   72,   78,
    QMetaType::Void, QMetaType::QString,   72,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Bool,  162,  163,
    QMetaType::Void, QMetaType::QString,  123,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,  168,  123,
    QMetaType::Void, QMetaType::QString,  168,
    QMetaType::Void, QMetaType::QString,  168,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,  173,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool, QMetaType::LongLong,  159,  176,  177,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,  159,  176,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,  159,  179,
    QMetaType::Void, QMetaType::QString,  159,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,  123,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,  187,
    QMetaType::Void, QMetaType::Int,  187,
    QMetaType::Void, QMetaType::Int,  187,
    QMetaType::Void, QMetaType::Int,  187,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,  168,  192,  193,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,  196,  197,
    QMetaType::Void, QMetaType::QString,  196,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString,  200,  196,  197,  201,  202,
    QMetaType::Void, QMetaType::QString,  196,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString,  200,  196,  197,  202,

 // properties: name, type, flags, notifyId, revision
     205, 0x80000000 | 206, 0x00015009, uint(0), 0,
     207, QMetaType::QString, 0x00015001, uint(1), 0,
     208, QMetaType::Bool, 0x00015001, uint(1), 0,
     209, QMetaType::Bool, 0x00015001, uint(2), 0,
     210, QMetaType::Bool, 0x00015001, uint(3), 0,
     211, QMetaType::Int, 0x00015001, uint(4), 0,
     212, 0x80000000 | 213, 0x00015009, uint(5), 0,
     214, QMetaType::QString, 0x00015001, uint(8), 0,
     215, QMetaType::QString, 0x00015001, uint(8), 0,
     216, QMetaType::QString, 0x00015001, uint(8), 0,
     217, QMetaType::QString, 0x00015001, uint(8), 0,
     218, QMetaType::QString, 0x00015001, uint(8), 0,
     219, 0x80000000 | 220, 0x00015009, uint(6), 0,
     221, QMetaType::QString, 0x00015001, uint(7), 0,
     222, QMetaType::QString, 0x00015001, uint(7), 0,
     223, QMetaType::QString, 0x00015001, uint(7), 0,
     224, QMetaType::QString, 0x00015001, uint(7), 0,
     225, QMetaType::Int, 0x00015001, uint(7), 0,
     226, QMetaType::QString, 0x00015001, uint(7), 0,
     227, QMetaType::Bool, 0x00015001, uint(7), 0,
     228, QMetaType::QString, 0x00015001, uint(9), 0,
     229, QMetaType::QString, 0x00015001, uint(9), 0,
     230, QMetaType::Bool, 0x00015001, uint(9), 0,
     231, QMetaType::Int, 0x00015001, uint(10), 0,
     232, QMetaType::Bool, 0x00015001, uint(22), 0,
     233, QMetaType::Int, 0x00015001, uint(22), 0,
     234, QMetaType::QString, 0x00015001, uint(22), 0,
     235, QMetaType::QString, 0x00015001, uint(22), 0,
     236, QMetaType::Bool, 0x00015001, uint(22), 0,
     237, QMetaType::Bool, 0x00015001, uint(22), 0,
     238, QMetaType::Bool, 0x00015001, uint(22), 0,
     239, QMetaType::Bool, 0x00015001, uint(22), 0,
     240, 0x80000000 | 241, 0x00015409, uint(-1), 0,
     242, 0x80000000 | 241, 0x00015409, uint(-1), 0,
     243, 0x80000000 | 241, 0x00015409, uint(-1), 0,
     244, 0x80000000 | 241, 0x00015409, uint(-1), 0,
     245, 0x80000000 | 246, 0x00015409, uint(-1), 0,
     247, 0x80000000 | 248, 0x00015409, uint(-1), 0,
     249, 0x80000000 | 250, 0x00015409, uint(-1), 0,
     251, QMetaType::Bool, 0x00015001, uint(11), 0,
     252, QMetaType::QString, 0x00015001, uint(12), 0,
     253, QMetaType::Bool, 0x00015001, uint(12), 0,
     254, QMetaType::Bool, 0x00015001, uint(13), 0,
     255, QMetaType::Bool, 0x00015001, uint(14), 0,
     256, QMetaType::Bool, 0x00015001, uint(15), 0,
     257, QMetaType::Bool, 0x00015001, uint(16), 0,
     258, QMetaType::Bool, 0x00015001, uint(17), 0,
     259, QMetaType::Bool, 0x00015001, uint(18), 0,
     260, QMetaType::Bool, 0x00015001, uint(19), 0,
     261, QMetaType::QString, 0x00015001, uint(20), 0,
     262, QMetaType::Bool, 0x00015001, uint(20), 0,
     263, QMetaType::QString, 0x00015001, uint(21), 0,
     264, QMetaType::Bool, 0x00015001, uint(21), 0,
     265, QMetaType::QString, 0x00015001, uint(23), 0,
     266, QMetaType::Bool, 0x00015001, uint(23), 0,
     267, QMetaType::Bool, 0x00015001, uint(24), 0,
     268, QMetaType::QString, 0x00015001, uint(24), 0,
     269, 0x80000000 | 270, 0x00015409, uint(-1), 0,
     271, 0x80000000 | 272, 0x00015409, uint(-1), 0,
     273, QMetaType::QString, 0x00015001, uint(25), 0,
     274, 0x80000000 | 137, 0x00015009, uint(26), 0,
     275, QMetaType::Bool, 0x00015001, uint(26), 0,
     276, QMetaType::Bool, 0x00015001, uint(27), 0,
     277, QMetaType::Bool, 0x00015001, uint(28), 0,
     278, QMetaType::Bool, 0x00015001, uint(29), 0,
     279, QMetaType::QString, 0x00015001, uint(29), 0,
     280, QMetaType::QString, 0x00015001, uint(29), 0,
     281, QMetaType::Bool, 0x00015001, uint(30), 0,
     282, QMetaType::Bool, 0x00015001, uint(30), 0,
     283, QMetaType::Bool, 0x00015001, uint(30), 0,
     284, QMetaType::Bool, 0x00015001, uint(30), 0,
     285, QMetaType::Bool, 0x00015001, uint(30), 0,

 // enums: name, alias, flags, count, data
     206,  206, 0x0,    4, 1650,
     213,  213, 0x0,    3, 1658,
     220,  220, 0x0,    2, 1664,

 // enum data: key, value
     286, uint(AppController::LoginPage),
     287, uint(AppController::RegisterPage),
     288, uint(AppController::ResetPage),
     289, uint(AppController::ChatPage),
     290, uint(AppController::ChatTabPage),
     291, uint(AppController::ContactTabPage),
     292, uint(AppController::SettingsTabPage),
     293, uint(AppController::ApplyRequestPane),
     294, uint(AppController::FriendInfoPane),

       0        // eod
};

Q_CONSTINIT const QMetaObject AppController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN13AppControllerE.offsetsAndSizes,
    qt_meta_data_ZN13AppControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13AppControllerE_t,
        // property 'page'
        QtPrivate::TypeAndForceComplete<Page, std::true_type>,
        // property 'tipText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'tipError'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'busy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'registerSuccessPage'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'registerCountdown'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'chatTab'
        QtPrivate::TypeAndForceComplete<ChatTab, std::true_type>,
        // property 'currentUserName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentUserNick'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentUserIcon'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentUserDesc'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentUserId'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'contactPane'
        QtPrivate::TypeAndForceComplete<ContactPane, std::true_type>,
        // property 'currentContactName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentContactNick'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentContactIcon'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentContactBack'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentContactSex'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'currentContactUserId'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hasCurrentContact'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentChatPeerName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentChatPeerIcon'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hasCurrentChat'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentDialogUid'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'hasCurrentGroup'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentGroupRole'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'currentGroupName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentGroupCode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentGroupCanChangeInfo'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentGroupCanInviteUsers'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentGroupCanManageAdmins'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentGroupCanBanUsers'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'dialogListModel'
        QtPrivate::TypeAndForceComplete<FriendListModel*, std::true_type>,
        // property 'chatListModel'
        QtPrivate::TypeAndForceComplete<FriendListModel*, std::true_type>,
        // property 'groupListModel'
        QtPrivate::TypeAndForceComplete<FriendListModel*, std::true_type>,
        // property 'contactListModel'
        QtPrivate::TypeAndForceComplete<FriendListModel*, std::true_type>,
        // property 'messageModel'
        QtPrivate::TypeAndForceComplete<ChatMessageModel*, std::true_type>,
        // property 'searchResultModel'
        QtPrivate::TypeAndForceComplete<SearchResultModel*, std::true_type>,
        // property 'applyRequestModel'
        QtPrivate::TypeAndForceComplete<ApplyRequestModel*, std::true_type>,
        // property 'searchPending'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'searchStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'searchStatusError'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'hasPendingApply'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'chatLoadingMore'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'canLoadMoreChats'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'privateHistoryLoading'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'canLoadMorePrivateHistory'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'contactLoadingMore'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'canLoadMoreContacts'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'authStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'authStatusError'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'settingsStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'settingsStatusError'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'groupStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'groupStatusError'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'mediaUploadInProgress'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'mediaUploadProgressText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'callSession'
        QtPrivate::TypeAndForceComplete<CallSessionModel*, std::true_type>,
        // property 'livekitBridge'
        QtPrivate::TypeAndForceComplete<LivekitBridge*, std::true_type>,
        // property 'currentDraftText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentPendingAttachments'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'hasPendingAttachments'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentDialogPinned'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'currentDialogMuted'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'hasPendingReply'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'replyTargetName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'replyPreviewText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'dialogsReady'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'contactsReady'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'groupsReady'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'applyReady'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'chatShellBusy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // enum 'Page'
        QtPrivate::TypeAndForceComplete<AppController::Page, std::true_type>,
        // enum 'ChatTab'
        QtPrivate::TypeAndForceComplete<AppController::ChatTab, std::true_type>,
        // enum 'ContactPane'
        QtPrivate::TypeAndForceComplete<AppController::ContactPane, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AppController, std::true_type>,
        // method 'pageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tipChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'busyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'registerSuccessPageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'registerCountdownChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chatTabChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'contactPaneChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentContactChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentUserChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentChatPeerChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentDialogUidChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchPendingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pendingApplyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chatLoadingMoreChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'canLoadMoreChatsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'privateHistoryLoadingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'canLoadMorePrivateHistoryChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'contactLoadingMoreChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'canLoadMoreContactsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'authStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'settingsStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentGroupChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'groupStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'mediaUploadStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentDraftTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentPendingAttachmentsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentDialogPinnedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentDialogMutedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pendingReplyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lazyBootstrapStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLoginHttpFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ReqId, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<ErrorCodes, std::false_type>,
        // method 'onRegisterHttpFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ReqId, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<ErrorCodes, std::false_type>,
        // method 'onResetHttpFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ReqId, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<ErrorCodes, std::false_type>,
        // method 'onSettingsHttpFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ReqId, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<ErrorCodes, std::false_type>,
        // method 'onCallHttpFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ReqId, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<ErrorCodes, std::false_type>,
        // method 'onTcpConnectFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onChatLoginFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onSwitchToChat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRelationBootstrapUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRegisterCountdownTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onHeartbeatTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onHeartbeatAck'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'onAddAuthFriend'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<AuthInfo>, std::false_type>,
        // method 'onAuthRsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<AuthRsp>, std::false_type>,
        // method 'onTextChatMsg'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<TextChatMsg>, std::false_type>,
        // method 'onUserSearch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<SearchInfo>, std::false_type>,
        // method 'onFriendApply'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<AddFriendApply>, std::false_type>,
        // method 'onNotifyOffline'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onConnectionClosed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGroupListUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGroupInvite'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onGroupApply'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'onGroupMemberChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onGroupChatMsg'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<GroupChatMsg>, std::false_type>,
        // method 'onGroupRsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ReqId, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onDialogListRsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onPrivateHistoryRsp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onPrivateMsgChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onPrivateReadAck'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onMessageStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onCallEvent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QJsonObject, std::false_type>,
        // method 'onLivekitRoomJoined'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLivekitRemoteTrackReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLivekitRoomDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onLivekitPermissionError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onLivekitMediaError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onLivekitReconnecting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'switchToLogin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'switchToRegister'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'switchToReset'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'switchChatTab'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'ensureContactsInitialized'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'ensureGroupsInitialized'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'ensureApplyInitialized'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'ensureChatListInitialized'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearTip'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectChatIndex'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'selectGroupIndex'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'selectDialogByUid'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'selectContactIndex'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'showApplyRequests'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'jumpChatWithCurrentContact'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadMoreChats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadMorePrivateHistory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadMoreContacts'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sendTextMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendCurrentComposerPayload'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendImageMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sendFileMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'removePendingAttachment'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clearPendingAttachments'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openExternalResource'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'searchUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clearSearchState'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestAddFriend'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'requestAddFriend'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'approveFriend'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'approveFriend'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clearAuthStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startVoiceChat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startVideoChat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'acceptIncomingCall'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'rejectIncomingCall'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'endCurrentCall'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleCallMuted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleCallCamera'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chooseAvatar'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveProfile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clearSettingsStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshGroupList'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createGroup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'createGroup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'inviteGroupMember'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'inviteGroupMember'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'applyJoinGroup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'applyJoinGroup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'reviewGroupApply'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'sendGroupTextMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendGroupImageMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sendGroupFileMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'editGroupMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'revokeGroupMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'forwardGroupMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'loadGroupHistory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateGroupAnnouncement'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateGroupIcon'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setGroupAdmin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'setGroupAdmin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'muteGroupMember'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'kickGroupMember'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'quitCurrentGroup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearGroupStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateCurrentDraft'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'toggleCurrentDialogPinned'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleCurrentDialogMuted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleDialogPinnedByUid'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'toggleDialogMutedByUid'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'markDialogReadByUid'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'clearDialogDraftByUid'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'beginReplyMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'cancelReplyMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'login'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'requestRegisterCode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'registerUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'requestResetCode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetPassword'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void AppController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AppController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->pageChanged(); break;
        case 1: _t->tipChanged(); break;
        case 2: _t->busyChanged(); break;
        case 3: _t->registerSuccessPageChanged(); break;
        case 4: _t->registerCountdownChanged(); break;
        case 5: _t->chatTabChanged(); break;
        case 6: _t->contactPaneChanged(); break;
        case 7: _t->currentContactChanged(); break;
        case 8: _t->currentUserChanged(); break;
        case 9: _t->currentChatPeerChanged(); break;
        case 10: _t->currentDialogUidChanged(); break;
        case 11: _t->searchPendingChanged(); break;
        case 12: _t->searchStatusChanged(); break;
        case 13: _t->pendingApplyChanged(); break;
        case 14: _t->chatLoadingMoreChanged(); break;
        case 15: _t->canLoadMoreChatsChanged(); break;
        case 16: _t->privateHistoryLoadingChanged(); break;
        case 17: _t->canLoadMorePrivateHistoryChanged(); break;
        case 18: _t->contactLoadingMoreChanged(); break;
        case 19: _t->canLoadMoreContactsChanged(); break;
        case 20: _t->authStatusChanged(); break;
        case 21: _t->settingsStatusChanged(); break;
        case 22: _t->currentGroupChanged(); break;
        case 23: _t->groupStatusChanged(); break;
        case 24: _t->mediaUploadStateChanged(); break;
        case 25: _t->currentDraftTextChanged(); break;
        case 26: _t->currentPendingAttachmentsChanged(); break;
        case 27: _t->currentDialogPinnedChanged(); break;
        case 28: _t->currentDialogMutedChanged(); break;
        case 29: _t->pendingReplyChanged(); break;
        case 30: _t->lazyBootstrapStateChanged(); break;
        case 31: _t->onLoginHttpFinished((*reinterpret_cast< std::add_pointer_t<ReqId>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<ErrorCodes>>(_a[3]))); break;
        case 32: _t->onRegisterHttpFinished((*reinterpret_cast< std::add_pointer_t<ReqId>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<ErrorCodes>>(_a[3]))); break;
        case 33: _t->onResetHttpFinished((*reinterpret_cast< std::add_pointer_t<ReqId>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<ErrorCodes>>(_a[3]))); break;
        case 34: _t->onSettingsHttpFinished((*reinterpret_cast< std::add_pointer_t<ReqId>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<ErrorCodes>>(_a[3]))); break;
        case 35: _t->onCallHttpFinished((*reinterpret_cast< std::add_pointer_t<ReqId>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<ErrorCodes>>(_a[3]))); break;
        case 36: _t->onTcpConnectFinished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 37: _t->onChatLoginFailed((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 38: _t->onSwitchToChat(); break;
        case 39: _t->onRelationBootstrapUpdated(); break;
        case 40: _t->onRegisterCountdownTimeout(); break;
        case 41: _t->onHeartbeatTimeout(); break;
        case 42: _t->onHeartbeatAck((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 43: _t->onAddAuthFriend((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<AuthInfo>>>(_a[1]))); break;
        case 44: _t->onAuthRsp((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<AuthRsp>>>(_a[1]))); break;
        case 45: _t->onTextChatMsg((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<TextChatMsg>>>(_a[1]))); break;
        case 46: _t->onUserSearch((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<SearchInfo>>>(_a[1]))); break;
        case 47: _t->onFriendApply((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<AddFriendApply>>>(_a[1]))); break;
        case 48: _t->onNotifyOffline(); break;
        case 49: _t->onConnectionClosed(); break;
        case 50: _t->onGroupListUpdated(); break;
        case 51: _t->onGroupInvite((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 52: _t->onGroupApply((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 53: _t->onGroupMemberChanged((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 54: _t->onGroupChatMsg((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<GroupChatMsg>>>(_a[1]))); break;
        case 55: _t->onGroupRsp((*reinterpret_cast< std::add_pointer_t<ReqId>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[3]))); break;
        case 56: _t->onDialogListRsp((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 57: _t->onPrivateHistoryRsp((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 58: _t->onPrivateMsgChanged((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 59: _t->onPrivateReadAck((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 60: _t->onMessageStatus((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 61: _t->onCallEvent((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 62: _t->onLivekitRoomJoined(); break;
        case 63: _t->onLivekitRemoteTrackReady(); break;
        case 64: _t->onLivekitRoomDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 65: _t->onLivekitPermissionError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 66: _t->onLivekitMediaError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 67: _t->onLivekitReconnecting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 68: _t->switchToLogin(); break;
        case 69: _t->switchToRegister(); break;
        case 70: _t->switchToReset(); break;
        case 71: _t->switchChatTab((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 72: _t->ensureContactsInitialized(); break;
        case 73: _t->ensureGroupsInitialized(); break;
        case 74: _t->ensureApplyInitialized(); break;
        case 75: _t->ensureChatListInitialized(); break;
        case 76: _t->clearTip(); break;
        case 77: _t->selectChatIndex((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 78: _t->selectGroupIndex((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 79: _t->selectDialogByUid((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 80: _t->selectContactIndex((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 81: _t->showApplyRequests(); break;
        case 82: _t->jumpChatWithCurrentContact(); break;
        case 83: _t->loadMoreChats(); break;
        case 84: _t->loadMorePrivateHistory(); break;
        case 85: _t->loadMoreContacts(); break;
        case 86: _t->sendTextMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 87: _t->sendCurrentComposerPayload((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 88: _t->sendImageMessage(); break;
        case 89: _t->sendFileMessage(); break;
        case 90: _t->removePendingAttachment((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 91: _t->clearPendingAttachments(); break;
        case 92: _t->openExternalResource((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 93: _t->searchUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 94: _t->clearSearchState(); break;
        case 95: _t->requestAddFriend((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[3]))); break;
        case 96: _t->requestAddFriend((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 97: _t->approveFriend((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[3]))); break;
        case 98: _t->approveFriend((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 99: _t->clearAuthStatus(); break;
        case 100: _t->startVoiceChat(); break;
        case 101: _t->startVideoChat(); break;
        case 102: _t->acceptIncomingCall(); break;
        case 103: _t->rejectIncomingCall(); break;
        case 104: _t->endCurrentCall(); break;
        case 105: _t->toggleCallMuted(); break;
        case 106: _t->toggleCallCamera(); break;
        case 107: _t->chooseAvatar(); break;
        case 108: _t->saveProfile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 109: _t->clearSettingsStatus(); break;
        case 110: _t->refreshGroupList(); break;
        case 111: _t->createGroup((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[2]))); break;
        case 112: _t->createGroup((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 113: _t->inviteGroupMember((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 114: _t->inviteGroupMember((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 115: _t->applyJoinGroup((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 116: _t->applyJoinGroup((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 117: _t->reviewGroupApply((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 118: _t->sendGroupTextMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 119: _t->sendGroupImageMessage(); break;
        case 120: _t->sendGroupFileMessage(); break;
        case 121: _t->editGroupMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 122: _t->revokeGroupMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 123: _t->forwardGroupMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 124: _t->loadGroupHistory(); break;
        case 125: _t->updateGroupAnnouncement((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 126: _t->updateGroupIcon(); break;
        case 127: _t->setGroupAdmin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[3]))); break;
        case 128: _t->setGroupAdmin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 129: _t->muteGroupMember((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 130: _t->kickGroupMember((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 131: _t->quitCurrentGroup(); break;
        case 132: _t->clearGroupStatus(); break;
        case 133: _t->updateCurrentDraft((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 134: _t->toggleCurrentDialogPinned(); break;
        case 135: _t->toggleCurrentDialogMuted(); break;
        case 136: _t->toggleDialogPinnedByUid((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 137: _t->toggleDialogMutedByUid((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 138: _t->markDialogReadByUid((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 139: _t->clearDialogDraftByUid((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 140: _t->beginReplyMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 141: _t->cancelReplyMessage(); break;
        case 142: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 143: _t->requestRegisterCode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 144: _t->registerUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[5]))); break;
        case 145: _t->requestResetCode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 146: _t->resetPassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::pageChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::tipChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::busyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::registerSuccessPageChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::registerCountdownChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::chatTabChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::contactPaneChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentContactChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentUserChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentChatPeerChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentDialogUidChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::searchPendingChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::searchStatusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::pendingApplyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::chatLoadingMoreChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::canLoadMoreChatsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::privateHistoryLoadingChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::canLoadMorePrivateHistoryChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::contactLoadingMoreChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::canLoadMoreContactsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 19;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::authStatusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 20;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::settingsStatusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 21;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentGroupChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 22;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::groupStatusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 23;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::mediaUploadStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 24;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentDraftTextChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 25;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentPendingAttachmentsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 26;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentDialogPinnedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 27;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentDialogMutedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 28;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::pendingReplyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 29;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::lazyBootstrapStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 30;
                return;
            }
        }
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 38:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ApplyRequestModel* >(); break;
        case 57:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CallSessionModel* >(); break;
        case 36:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ChatMessageModel* >(); break;
        case 35:
        case 34:
        case 33:
        case 32:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< FriendListModel* >(); break;
        case 58:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< LivekitBridge* >(); break;
        case 37:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< SearchResultModel* >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< Page*>(_v) = _t->page(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->tipText(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->tipError(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->busy(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->registerSuccessPage(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->registerCountdown(); break;
        case 6: *reinterpret_cast< ChatTab*>(_v) = _t->chatTab(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->currentUserName(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->currentUserNick(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->currentUserIcon(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->currentUserDesc(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->currentUserId(); break;
        case 12: *reinterpret_cast< ContactPane*>(_v) = _t->contactPane(); break;
        case 13: *reinterpret_cast< QString*>(_v) = _t->currentContactName(); break;
        case 14: *reinterpret_cast< QString*>(_v) = _t->currentContactNick(); break;
        case 15: *reinterpret_cast< QString*>(_v) = _t->currentContactIcon(); break;
        case 16: *reinterpret_cast< QString*>(_v) = _t->currentContactBack(); break;
        case 17: *reinterpret_cast< int*>(_v) = _t->currentContactSex(); break;
        case 18: *reinterpret_cast< QString*>(_v) = _t->currentContactUserId(); break;
        case 19: *reinterpret_cast< bool*>(_v) = _t->hasCurrentContact(); break;
        case 20: *reinterpret_cast< QString*>(_v) = _t->currentChatPeerName(); break;
        case 21: *reinterpret_cast< QString*>(_v) = _t->currentChatPeerIcon(); break;
        case 22: *reinterpret_cast< bool*>(_v) = _t->hasCurrentChat(); break;
        case 23: *reinterpret_cast< int*>(_v) = _t->currentDialogUid(); break;
        case 24: *reinterpret_cast< bool*>(_v) = _t->hasCurrentGroup(); break;
        case 25: *reinterpret_cast< int*>(_v) = _t->currentGroupRole(); break;
        case 26: *reinterpret_cast< QString*>(_v) = _t->currentGroupName(); break;
        case 27: *reinterpret_cast< QString*>(_v) = _t->currentGroupCode(); break;
        case 28: *reinterpret_cast< bool*>(_v) = _t->currentGroupCanChangeInfo(); break;
        case 29: *reinterpret_cast< bool*>(_v) = _t->currentGroupCanInviteUsers(); break;
        case 30: *reinterpret_cast< bool*>(_v) = _t->currentGroupCanManageAdmins(); break;
        case 31: *reinterpret_cast< bool*>(_v) = _t->currentGroupCanBanUsers(); break;
        case 32: *reinterpret_cast< FriendListModel**>(_v) = _t->dialogListModel(); break;
        case 33: *reinterpret_cast< FriendListModel**>(_v) = _t->chatListModel(); break;
        case 34: *reinterpret_cast< FriendListModel**>(_v) = _t->groupListModel(); break;
        case 35: *reinterpret_cast< FriendListModel**>(_v) = _t->contactListModel(); break;
        case 36: *reinterpret_cast< ChatMessageModel**>(_v) = _t->messageModel(); break;
        case 37: *reinterpret_cast< SearchResultModel**>(_v) = _t->searchResultModel(); break;
        case 38: *reinterpret_cast< ApplyRequestModel**>(_v) = _t->applyRequestModel(); break;
        case 39: *reinterpret_cast< bool*>(_v) = _t->searchPending(); break;
        case 40: *reinterpret_cast< QString*>(_v) = _t->searchStatusText(); break;
        case 41: *reinterpret_cast< bool*>(_v) = _t->searchStatusError(); break;
        case 42: *reinterpret_cast< bool*>(_v) = _t->hasPendingApply(); break;
        case 43: *reinterpret_cast< bool*>(_v) = _t->chatLoadingMore(); break;
        case 44: *reinterpret_cast< bool*>(_v) = _t->canLoadMoreChats(); break;
        case 45: *reinterpret_cast< bool*>(_v) = _t->privateHistoryLoading(); break;
        case 46: *reinterpret_cast< bool*>(_v) = _t->canLoadMorePrivateHistory(); break;
        case 47: *reinterpret_cast< bool*>(_v) = _t->contactLoadingMore(); break;
        case 48: *reinterpret_cast< bool*>(_v) = _t->canLoadMoreContacts(); break;
        case 49: *reinterpret_cast< QString*>(_v) = _t->authStatusText(); break;
        case 50: *reinterpret_cast< bool*>(_v) = _t->authStatusError(); break;
        case 51: *reinterpret_cast< QString*>(_v) = _t->settingsStatusText(); break;
        case 52: *reinterpret_cast< bool*>(_v) = _t->settingsStatusError(); break;
        case 53: *reinterpret_cast< QString*>(_v) = _t->groupStatusText(); break;
        case 54: *reinterpret_cast< bool*>(_v) = _t->groupStatusError(); break;
        case 55: *reinterpret_cast< bool*>(_v) = _t->mediaUploadInProgress(); break;
        case 56: *reinterpret_cast< QString*>(_v) = _t->mediaUploadProgressText(); break;
        case 57: *reinterpret_cast< CallSessionModel**>(_v) = _t->callSession(); break;
        case 58: *reinterpret_cast< LivekitBridge**>(_v) = _t->livekitBridge(); break;
        case 59: *reinterpret_cast< QString*>(_v) = _t->currentDraftText(); break;
        case 60: *reinterpret_cast< QVariantList*>(_v) = _t->currentPendingAttachments(); break;
        case 61: *reinterpret_cast< bool*>(_v) = _t->hasPendingAttachments(); break;
        case 62: *reinterpret_cast< bool*>(_v) = _t->currentDialogPinned(); break;
        case 63: *reinterpret_cast< bool*>(_v) = _t->currentDialogMuted(); break;
        case 64: *reinterpret_cast< bool*>(_v) = _t->hasPendingReply(); break;
        case 65: *reinterpret_cast< QString*>(_v) = _t->replyTargetName(); break;
        case 66: *reinterpret_cast< QString*>(_v) = _t->replyPreviewText(); break;
        case 67: *reinterpret_cast< bool*>(_v) = _t->dialogsReady(); break;
        case 68: *reinterpret_cast< bool*>(_v) = _t->contactsReady(); break;
        case 69: *reinterpret_cast< bool*>(_v) = _t->groupsReady(); break;
        case 70: *reinterpret_cast< bool*>(_v) = _t->applyReady(); break;
        case 71: *reinterpret_cast< bool*>(_v) = _t->chatShellBusy(); break;
        default: break;
        }
    }
}

const QMetaObject *AppController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AppController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13AppControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AppController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 147)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 147;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 147)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 147;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 72;
    }
    return _id;
}

// SIGNAL 0
void AppController::pageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AppController::tipChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AppController::busyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AppController::registerSuccessPageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AppController::registerCountdownChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AppController::chatTabChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AppController::contactPaneChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void AppController::currentContactChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void AppController::currentUserChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void AppController::currentChatPeerChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void AppController::currentDialogUidChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void AppController::searchPendingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void AppController::searchStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void AppController::pendingApplyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void AppController::chatLoadingMoreChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void AppController::canLoadMoreChatsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void AppController::privateHistoryLoadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void AppController::canLoadMorePrivateHistoryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 17, nullptr);
}

// SIGNAL 18
void AppController::contactLoadingMoreChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}

// SIGNAL 19
void AppController::canLoadMoreContactsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 19, nullptr);
}

// SIGNAL 20
void AppController::authStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}

// SIGNAL 21
void AppController::settingsStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 21, nullptr);
}

// SIGNAL 22
void AppController::currentGroupChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 22, nullptr);
}

// SIGNAL 23
void AppController::groupStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 23, nullptr);
}

// SIGNAL 24
void AppController::mediaUploadStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void AppController::currentDraftTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 25, nullptr);
}

// SIGNAL 26
void AppController::currentPendingAttachmentsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}

// SIGNAL 27
void AppController::currentDialogPinnedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 27, nullptr);
}

// SIGNAL 28
void AppController::currentDialogMutedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void AppController::pendingReplyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 29, nullptr);
}

// SIGNAL 30
void AppController::lazyBootstrapStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 30, nullptr);
}
QT_WARNING_POP
