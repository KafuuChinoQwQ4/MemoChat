#include "MainWindow.h"
#include "TcpMgr.h" // [新增] 必须包含，用于连接登录成功信号
#include <QIcon>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chatDlg(nullptr) // 初始化为空
{
    // 设置窗口固定大小 300x500 (登录状态)
    setFixedSize(300, 500);
    setWindowTitle("MemoChat");
    setWindowIcon(QIcon(":/res/KafuuChino.png")); 

    // 1. 创建堆栈部件作为中心部件
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // 2. 创建并添加页面
    m_loginDlg = new LoginDialog(this);
    m_regDlg = new RegisterDialog(this);
    m_resetDlg = new ResetDialog(this);

    m_stackedWidget->addWidget(m_loginDlg); // Index 0
    m_stackedWidget->addWidget(m_regDlg);   // Index 1
    m_stackedWidget->addWidget(m_resetDlg); // Index 2

    // 3. 初始显示登录页
    m_stackedWidget->setCurrentWidget(m_loginDlg);

    // 4. 连接页面切换信号槽
    // 登录 -> 注册
    connect(m_loginDlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    // 注册 -> 登录
    connect(m_regDlg, &RegisterDialog::switchLogin, this, &MainWindow::SlotSwitchLogin);
    // 登录 -> 重置
    connect(m_loginDlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    // 重置 -> 登录
    connect(m_resetDlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin);

    // 5. [新增] 连接 TcpMgr 的跳转信号
    // 当 TcpMgr 解析到登录成功的包后，会发出 sig_swich_chatdlg 信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
}

MainWindow::~MainWindow() {
    // Qt 的父子对象机制会自动析构子对象
}

void MainWindow::SlotSwitchReg() {
    m_stackedWidget->setCurrentWidget(m_regDlg);
}

void MainWindow::SlotSwitchLogin() {
    m_stackedWidget->setCurrentWidget(m_loginDlg);
}

void MainWindow::SlotSwitchReset() {
    m_stackedWidget->setCurrentWidget(m_resetDlg);
}

// [新增] 切换到聊天界面
void MainWindow::SlotSwitchChat() {
    qDebug() << "Switching to Chat Window...";

    // 懒加载：第一次切换时才创建 ChatDialog
    if(!m_chatDlg) {
        m_chatDlg = new ChatDialog(this);
        m_stackedWidget->addWidget(m_chatDlg); // Index 3
    }

    // 1. 切换堆栈页面
    m_stackedWidget->setCurrentWidget(m_chatDlg);

    // 2. [关键] 调整窗口大小
    // 登录界面是竖长的，聊天界面通常是宽大的
    setFixedSize(900, 700); 
    
    // 3. 居中显示 (可选)
    // 获取屏幕几何信息让窗口居中会体验更好，这里简单处理
    // this->move(screen()->geometry().center() - this->rect().center());
}