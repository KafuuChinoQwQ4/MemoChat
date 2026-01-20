#pragma once
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QStackedLayout> // 必须包含
#include <QTimer>         // 必须包含
#include "TimerBtn.h"     
#include "ClickedLabel.h" 
#include "global.h"

class RegisterDialog : public QDialog {
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog(); // 析构函数声明

private slots:
    void onGetCodeClicked();
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
    void onRegisterClicked();

    // === 校验槽函数 (改为 void 类型以匹配 cpp) ===
    void checkUserValid();
    void checkEmailValid();
    void checkPassValid();
    void checkConfirmValid();
    void checkVarifyValid();

signals:
    void switchLogin();

private:
    void initUI();
    void initHttpHandlers();
    void showTip(QString str, bool b_ok);
    
    // === 错误管理函数 ===
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);

    // === 成员变量 ===
    // 界面控件
    QLineEdit *m_userEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_passEdit;
    QLineEdit *m_confirmEdit;
    QLineEdit *m_codeEdit;
    
    TimerBtn *m_getCodeBtn; 
    QPushButton *m_registerBtn;
    QPushButton *m_cancelBtn;
    QLabel *m_errTip;
    
    // 密码可见性控件 
    ClickedLabel *m_passVisible;
    ClickedLabel *m_confirmVisible;

    // 页面切换 (新增的成员变量)
    QStackedLayout *m_stackedLayout;
    QWidget *m_pageRegister;
    QWidget *m_pageSuccess;
    QLabel *m_successTip; 
    QTimer *m_countdownTimer; 
    int m_countdown; 

    // 错误缓存 Map
    QMap<TipErr, QString> _tip_errs;
    
    // 业务处理器
    QMap<ReqId, std::function<void(const QJsonObject&)>> m_handlers;
};