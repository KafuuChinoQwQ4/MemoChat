#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "LoginDialog.h"
#include "RegisterDialog.h"
#include "ResetDialog.h" // <--- 必须包含这个

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchReset(); // <--- 切换重置槽函数

private:
    QStackedWidget *m_stackedWidget;
    
    LoginDialog *m_loginDlg;
    RegisterDialog *m_regDlg;
    ResetDialog *m_resetDlg; // <--- 声明成员变量
};