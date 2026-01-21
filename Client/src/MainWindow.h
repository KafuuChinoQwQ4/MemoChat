#pragma once
#include <QMainWindow>
#include <QStackedWidget> // 必须包含这个
#include "LoginDialog.h"
#include "RegisterDialog.h"
#include "ResetDialog.h"
#include "ChatDialog.h" 

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchReset();
    void SlotSwitchChat();

private:
    // 这里必须声明 .cpp 中用到的所有成员变量
    QStackedWidget *m_stackedWidget;
    LoginDialog *m_loginDlg;
    RegisterDialog *m_regDlg;
    ResetDialog *m_resetDlg;
    ChatDialog *m_chatDlg;
};