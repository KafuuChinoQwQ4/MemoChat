#pragma once
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "ClickedLabel.h" // 引用 点击标签
#include "global.h"

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private slots:
    void onLoginClicked(); // 登录按钮槽函数
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err); // 登录回包处理

signals:
    void switchRegister();
    void switchReset();

private:
    void initUI();
    void initHttpHandlers();
    void showTip(QString str, bool b_ok); // 显示错误提示
    bool checkUserValid(); // 校验用户名
    bool checkPwdValid();  // 校验密码
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);

    // UI控件
    QLabel *m_logoLabel; // 如果你有logo的话
    QLabel *m_errTip;    // [新增] 错误提示标签
    
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;
    
    ClickedLabel *m_passVisible; // [新增] 密码可见性小眼睛
    ClickedLabel *m_forgetLabel; 

    QPushButton *m_loginBtn;
    QPushButton *m_regBtn;

    // 错误状态管理
    QMap<TipErr, QString> _tip_errs;
    QMap<ReqId, std::function<void(const QJsonObject&)>> m_handlers;
};