#pragma once
#include <QWidget>
#include <functional>
#include <QStyle>
#include <QRegularExpression>
#include <QByteArray>

extern std::function<void(QWidget*)> repolish;
extern QString gate_url_prefix;

/**
 * @brief 刷新 QSS 样式
 */
extern std::function<void(QWidget*)> repolish;
extern QString gate_url_prefix;

/**
 * @brief 请求 ID 枚举
 */
enum ReqId {
    ID_GET_VARIFY_CODE = 1001, 
    ID_REG_USER = 1002,   
    ID_RESET_PWD = 1003,   
    ID_LOGIN_USER = 1004,
    ID_CHAT_LOGIN = 1005,
    ID_CHAT_LOGIN_RSP = 1006,
    ID_SEARCH_USER_REQ = 1007, // [新增] 搜索用户请求
    ID_SEARCH_USER_RSP = 1008, // [建议新增] 搜索用户回包
    ID_ADD_FRIEND_REQ = 1009,
    ID_ADD_FRIEND_RSP = 1010,
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,
    ID_AUTH_FRIEND_REQ = 1012,
    ID_AUTH_FRIEND_RSP = 1013,
    ID_NOTIFY_AUTH_FRIEND_REQ = 1014,
    ID_TEXT_CHAT_MSG_REQ = 1015,
    ID_TEXT_CHAT_MSG_RSP = 1016,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1017,
};                                   

/**
 * @brief 模块分类
 */
enum Modules {
    REGISTERMOD = 0,
    RESETMOD = 1,
    LOGINMOD = 2,
};

/**
 * @brief 错误码
 */
enum ErrorCodes {
    SUCCESS = 0,
    ERR_JSON = 1,
    ERR_NETWORK = 2,
    RPC_FAILED = 1002,
    VARIFY_EXPIRED = 1003,
    VARIFY_CODE_ERR = 1004,
    USER_EXIST = 1005,
    PASSWD_ERR = 1006,
    EMAIL_NOT_MATCH = 1007,  // <--- 核心：邮箱不匹配
    PASSWD_UP_FAILED = 1008,
    USER_NOT_EXIST = 1009,
    TOKEN_INVALID = 1010
};

/**
 * @brief 点击状态
 */

enum ClickLbState{
    Normal = 0,
    Selected = 1
};

/**
 * @brief 错误状态枚举
 */

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_VARIFY_ERR = 4,
    TIP_USER_ERR = 5
};

struct ServerInfo {
    int Uid;
    QString Host;
    QString Port;
    QString Token;
};

/**
 * @brief 聊天角色
 */
 enum ChatRole {
    Self,
    Other
};

/**
 * @brief 列表条目类型
 */
 enum ListItemType {
    Pp_CHAT_USER_ITEM = 0,
    Pp_CONTACT_USER_ITEM = 1, // [Modified] 确保有这个
    Pp_SEARCH_USER_ITEM = 2,
    Pp_ADD_USER_TIP_ITEM = 3,
    Pp_INVALID_ITEM = 4,
    Pp_GROUP_TIP_ITEM = 5,    // [New] 分组标题
    Pp_APPLY_FRIEND_ITEM = 6  // [New] 好友申请
};

/**
 * @brief 搜索到的用户信息
 */
struct SearchInfo {
    // [修改] 增加 icon 参数
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex, QString icon)
        : _uid(uid), _name(name), _nick(nick), _desc(desc), _sex(sex), _icon(icon) {}
    int _uid;
    QString _name;
    QString _nick;
    QString _desc;
    int _sex;
    QString _icon; // [新增]
};
