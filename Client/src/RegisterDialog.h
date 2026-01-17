#pragma once
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <functional>
#include <QJsonObject>
#include "global.h"

class RegisterDialog : public QDialog {
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget *parent = nullptr);

private slots:
    void onGetCodeClicked(); // 获取验证码按钮点击
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err); // 接收 HTTP 回包

signals:
    void switchLogin();

private:
    void initUI();
    void initHttpHandlers(); // 初始化业务逻辑处理器
    void showTip(QString str, bool b_ok); // 显示错误/成功提示

    // UI 控件
    QLineEdit *m_userEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_passEdit;
    QLineEdit *m_confirmEdit;
    QLineEdit *m_codeEdit; // 验证码输入框
    QPushButton *m_getCodeBtn;
    QPushButton *m_registerBtn;
    QPushButton *m_cancelBtn;
    QLabel *m_errTip; // 错误提示标签

    // 业务处理器映射表
    QMap<ReqId, std::function<void(const QJsonObject&)>> m_handlers;
};