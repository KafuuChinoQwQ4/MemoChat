#include "LoginDialog.h"
#include <QPixmap>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    // 设置去边框，因为我们要嵌入到 MainWindow 里
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint); 
    initUI();
    
    // 连接信号：点击注册按钮 -> 发送 switchRegister 信号
    connect(m_regBtn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
}

void LoginDialog::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 1. Logo 图片
    m_logoLabel = new QLabel(this);
    QPixmap pixmap(":/res/KafuuChino.png"); // 加载资源
    m_logoLabel->setPixmap(pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    
    // 2. 输入框
    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("请输入用户/邮箱");
    
    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("请输入密码");
    m_passEdit->setEchoMode(QLineEdit::Password);

    // 3. 按钮
    m_loginBtn = new QPushButton("登录", this);
    m_regBtn = new QPushButton("注册", this);

    // 4. 添加到布局 (增加弹簧 addStretch 让布局更好看)
    mainLayout->addStretch();
    mainLayout->addWidget(m_logoLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_userEdit);
    mainLayout->addWidget(m_passEdit);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_loginBtn);
    mainLayout->addWidget(m_regBtn);
    mainLayout->addStretch();
}