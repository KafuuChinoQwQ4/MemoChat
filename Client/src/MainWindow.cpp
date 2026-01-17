#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置窗口固定大小 300x500
    setFixedSize(300, 500);
    setWindowTitle("MemoChat");
    setWindowIcon(QIcon(":/res/ice.png")); // 设置图标

    // 1. 创建堆栈部件作为中心部件
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // 2. 创建并添加页面
    m_loginDlg = new LoginDialog(this);
    m_regDlg = new RegisterDialog(this);

    m_stackedWidget->addWidget(m_loginDlg); // Index 0
    m_stackedWidget->addWidget(m_regDlg);   // Index 1

    // 3. 初始显示登录页
    m_stackedWidget->setCurrentWidget(m_loginDlg);

    // 4. 连接信号槽
    // 登录界面 -> 切换注册
    connect(m_loginDlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    // 注册界面 -> 返回登录
    connect(m_regDlg, &RegisterDialog::switchLogin, this, &MainWindow::SlotSwitchLogin);
}

MainWindow::~MainWindow() {
    // Qt 的父子对象机制会自动析构子对象，无需手动 delete
}

void MainWindow::SlotSwitchReg() {
    m_stackedWidget->setCurrentWidget(m_regDlg);
}

void MainWindow::SlotSwitchLogin() {
    m_stackedWidget->setCurrentWidget(m_loginDlg);
}