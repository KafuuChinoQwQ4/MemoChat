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