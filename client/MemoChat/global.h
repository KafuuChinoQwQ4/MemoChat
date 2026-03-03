#ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include "QStyle"
#include <memory>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QDir>
#include <QSettings>

extern std::function<void(QWidget*)> repolish;


extern std::function<QString(QString)> xorString;

enum ReqId{
    ID_GET_VARIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_RESET_PWD = 1003,
    ID_LOGIN_USER = 1004,
    ID_CHAT_LOGIN = 1005,
    ID_CHAT_LOGIN_RSP= 1006,
    ID_SEARCH_USER_REQ = 1007,
    ID_SEARCH_USER_RSP = 1008,
    ID_ADD_FRIEND_REQ = 1009,
    ID_ADD_FRIEND_RSP = 1010,
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,
    ID_AUTH_FRIEND_REQ = 1013,
    ID_AUTH_FRIEND_RSP = 1014,
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,
    ID_TEXT_CHAT_MSG_REQ  = 1017,
    ID_TEXT_CHAT_MSG_RSP  = 1018,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
    ID_NOTIFY_OFF_LINE_REQ = 1021,
    ID_HEART_BEAT_REQ = 1023,
    ID_HEARTBEAT_RSP = 1024,
    ID_UPDATE_PROFILE = 1025,
    ID_CREATE_GROUP_REQ = 1031,
    ID_CREATE_GROUP_RSP = 1032,
    ID_GET_GROUP_LIST_REQ = 1033,
    ID_GET_GROUP_LIST_RSP = 1034,
    ID_INVITE_GROUP_MEMBER_REQ = 1035,
    ID_INVITE_GROUP_MEMBER_RSP = 1036,
    ID_NOTIFY_GROUP_INVITE_REQ = 1037,
    ID_APPLY_JOIN_GROUP_REQ = 1038,
    ID_APPLY_JOIN_GROUP_RSP = 1039,
    ID_NOTIFY_GROUP_APPLY_REQ = 1040,
    ID_REVIEW_GROUP_APPLY_REQ = 1041,
    ID_REVIEW_GROUP_APPLY_RSP = 1042,
    ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ = 1043,
    ID_GROUP_CHAT_MSG_REQ = 1044,
    ID_GROUP_CHAT_MSG_RSP = 1045,
    ID_NOTIFY_GROUP_CHAT_MSG_REQ = 1046,
    ID_GROUP_HISTORY_REQ = 1047,
    ID_GROUP_HISTORY_RSP = 1048,
    ID_UPDATE_GROUP_ANNOUNCEMENT_REQ = 1049,
    ID_UPDATE_GROUP_ANNOUNCEMENT_RSP = 1050,
    ID_MUTE_GROUP_MEMBER_REQ = 1051,
    ID_MUTE_GROUP_MEMBER_RSP = 1052,
    ID_SET_GROUP_ADMIN_REQ = 1053,
    ID_SET_GROUP_ADMIN_RSP = 1054,
    ID_KICK_GROUP_MEMBER_REQ = 1055,
    ID_KICK_GROUP_MEMBER_RSP = 1056,
    ID_QUIT_GROUP_REQ = 1057,
    ID_QUIT_GROUP_RSP = 1058,
    ID_PRIVATE_HISTORY_REQ = 1059,
    ID_PRIVATE_HISTORY_RSP = 1060,
    ID_UPDATE_GROUP_ICON_REQ = 1061,
    ID_UPDATE_GROUP_ICON_RSP = 1062,
};

enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,
    ERR_NETWORK = 2,
    ERR_VERSION_TOO_LOW = 1014,
};

enum Modules{
    REGISTERMOD = 0,
    RESETMOD = 1,
    LOGINMOD = 2,
    SETTINGSMOD = 3,
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6
};

enum ClickLbState{
    Normal = 0,
    Selected = 1
};


extern QString gate_url_prefix;

#define MEMOCHAT_CLIENT_VERSION "2.0.0"


struct ServerInfo{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

enum class ChatRole
{

    Self,
    Other
};

struct MsgInfo{
    QString msgFlag;//"text,image,file"
    QString content;
    QPixmap pixmap;
};


enum ChatUIMode{
    SearchMode,
    ChatMode,
    ContactMode,
    SettingsMode,
};


enum ListItemType{
    CHAT_USER_ITEM,
    CONTACT_USER_ITEM,
    SEARCH_USER_ITEM,
    ADD_USER_TIP_ITEM,
    INVALID_ITEM,
    GROUP_TIP_ITEM,
    LINE_ITEM,
    APPLY_FRIEND_ITEM,
};


const int MIN_APPLY_LABEL_ED_LEN = 40;

const QString add_prefix = "添加标签 ";

const int  tip_offset = 5;


const std::vector<QString>  strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                            "You have to love yourself",
                            "My love is written in the wind ever since the whole world is you"};

const std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg"
};

const std::vector<QString> names = {
    "HanMeiMei",
    "Lily",
    "Ben",
    "Androw",
    "Max",
    "Summer",
    "Candy",
    "Hunter"
};

const int CHAT_COUNT_PER_PAGE = 13;


#endif // GLOBAL_H
