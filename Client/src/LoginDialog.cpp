#include "LoginDialog.h"
#include "HttpMgr.h"
#include <QDebug>
#include <QRegularExpression>
#include <QPainter>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    initUI();
    initHttpHandlers();

    // 连接 HttpMgr 登录信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, 
            this, &LoginDialog::slot_login_mod_finish);
}

LoginDialog::~LoginDialog() {}

void LoginDialog::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 1. [新增] 错误提示标签 (默认隐藏或显示空)
    m_errTip = new QLabel(this);
    m_errTip->setAlignment(Qt::AlignCenter);
    m_errTip->setProperty("state", "normal"); 
    // 在 stylesheet.qss 中定义 QLabel#m_errTip[state='err'] { color: red; }

    // 用户名输入
    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("用户名");

    // 2. [新增] 密码输入框 + 小眼睛布局
    QHBoxLayout *passLayout = new QHBoxLayout();
    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("密码");
    m_passEdit->setEchoMode(QLineEdit::Password); // 默认密文

    m_passVisible = new ClickedLabel(this);
    m_passVisible->setObjectName("pass_visible");
    m_passVisible->setFixedSize(20, 20);
    m_passVisible->SetState("unvisible", "unvisible_hover", "", "visible", "visible_hover", "");

    passLayout->addWidget(m_passEdit);
    passLayout->addWidget(m_passVisible);

    // 忘记密码标签
    m_forgetLabel = new ClickedLabel(this);
    m_forgetLabel->setText("忘记密码");
    m_forgetLabel->setCursor(Qt::PointingHandCursor);
    m_forgetLabel->SetState("normal", "hover", "", "selected", "selected_hover", "");

    // 按钮
    m_loginBtn = new QPushButton("登录", this);
    m_regBtn = new QPushButton("注册", this);

    // 添加到主布局
    mainLayout->addWidget(m_errTip);
    mainLayout->addWidget(m_userEdit);
    mainLayout->addLayout(passLayout); // 加入密码布局
    mainLayout->addWidget(m_forgetLabel, 0, Qt::AlignRight);
    mainLayout->addWidget(m_loginBtn);
    mainLayout->addWidget(m_regBtn);

    // === 信号连接 ===
    
    // [新增] 密码可见性切换逻辑
    connect(m_passVisible, &ClickedLabel::clicked, this, [this]() {
        if (m_passVisible->GetCurState() == ClickLbState::Normal) {
            m_passEdit->setEchoMode(QLineEdit::Password);
        } else {
            m_passEdit->setEchoMode(QLineEdit::Normal);
        }
    });

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_regBtn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
    connect(m_forgetLabel, &ClickedLabel::clicked, this, &LoginDialog::switchReset);
}

// 登录按钮点击逻辑
void LoginDialog::onLoginClicked() {
    qDebug() << "login btn clicked";
    
    // 基础校验
    if(!checkUserValid() || !checkPwdValid()) {
        return;
    }

    // 发送登录请求
    QJsonObject json_obj;
    json_obj["user"] = m_userEdit->text();
    json_obj["passwd"] = m_passEdit->text();
    
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_login"),
        json_obj, 
        ReqId::ID_LOGIN_USER, 
        Modules::LOGINMOD
    );
}

// 校验逻辑
bool LoginDialog::checkUserValid() {
    if (m_userEdit->text().isEmpty()) {
        showTip("用户名不能为空", false);
        return false;
    }
    return true;
}

bool LoginDialog::checkPwdValid() {
    if (m_passEdit->text().isEmpty()) {
        showTip("密码不能为空", false);
        return false;
    }
    return true;
}

void LoginDialog::showTip(QString str, bool b_ok) {
    if (b_ok) m_errTip->setProperty("state", "normal");
    else m_errTip->setProperty("state", "err");
    m_errTip->setText(str);
    repolish(m_errTip);
}

// HTTP 回包处理
void LoginDialog::initHttpHandlers() {
    m_handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("登录失败"), false);
            
            // 修正：使用大写枚举值
            if(error == ErrorCodes::USER_NOT_EXIST) { // 改成 USER_NOT_EXIST
                showTip(tr("用户不存在"), false);
            }
            else if(error == ErrorCodes::PASSWD_ERR) { // 改成 PASSWD_ERR
                showTip(tr("密码错误"), false);
            }
            return;
        }
        showTip(tr("登录成功"), true);
        qDebug() << "Login Success, Server info:" << jsonObj;
    });
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err) {
    if(err != ErrorCodes::SUCCESS) { showTip("网络请求错误", false); return; }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()) { showTip("JSON解析错误", false); return; }
    if(m_handlers.contains(id)) m_handlers[id](jsonDoc.object());
}