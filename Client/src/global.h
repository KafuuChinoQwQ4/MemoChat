#pragma once
#include <QWidget>
#include <functional>
#include <QStyle>
#include <QRegularExpression>
#include <QByteArray>

/**
 * @brief 刷新 QSS 样式
 */
extern std::function<void(QWidget*)> repolish;
extern QString gate_url_prefix;

/**
 * @brief 请求 ID 枚举
 */
enum ReqId {
    ID_GET_VARIFY_CODE = 1001, // 获取验证码
    ID_REG_USER = 1002,        // 注册用户
};

/**
 * @brief 模块分类
 */
enum Modules {
    REGISTERMOD = 0,
};

/**
 * @brief 错误码
 */
enum ErrorCodes {
    SUCCESS = 0,
    ERR_JSON = 1,
    ERR_NETWORK = 2,
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