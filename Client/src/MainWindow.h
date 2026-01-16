#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "LoginDialog.h"
#include "RegisterDialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void SlotSwitchReg(); // 切换到注册
    void SlotSwitchLogin(); // 切换回登录

private:
    // 使用 QStackedWidget 管理页面
    QStackedWidget *m_stackedWidget;
    
    LoginDialog *m_loginDlg;
    RegisterDialog *m_regDlg;
};