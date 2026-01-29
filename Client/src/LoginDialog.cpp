#include "LoginDialog.h"
#include "HttpMgr.h"
#include "TcpMgr.h" // [新增] 引入 TCP 管理类
#include <QDebug>
#include <QRegularExpression>
#include <QPainter>
#include <QJsonDocument> // [新增] 用于构建 JSON 字符串

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    initUI();
    initHttpHandlers();

    // 连接 HttpMgr 登录信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, 
            this, &LoginDialog::slot_login_mod_finish);
            
    // === [新增] 连接 TcpMgr ===
    // 1. 本界面发出"去连接"信号 -> TcpMgr 执行连接
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
    
    // 2. TcpMgr 发出"连接结果"信号 -> 本界面处理 (如果成功则发送登录包)
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);

    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed, this, [this](int err){
        QString result = QString("登录失败, err is %1").arg(err);
        showTip(result, false);
    });

    // [新增] 连接 TCP 登录成功跳转信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_swich_chatdlg, this, [this](){
        qDebug() << "Login ChatServer Success, switching to ChatDialog...";
        showTip("登录成功，进入聊天...", true);
        // 这里暂时先隐藏窗口，Day 18 会讲创建 ChatDialog
        this->hide(); 
        // emit switchChat(); // 将来这里会发信号给 MainWindow 切换页面
    });
}

LoginDialog::~LoginDialog() {}

void LoginDialog::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 1. 错误提示标签
    m_errTip = new QLabel(this);
    m_errTip->setObjectName("m_errTip"); 
    m_errTip->setAlignment(Qt::AlignCenter);
    m_errTip->setProperty("state", "normal"); 

    // 用户名输入
    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("用户名");

    // 2. 密码输入框 + 小眼睛布局
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
    
    // 密码可见性切换逻辑
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
            
            if(error == ErrorCodes::USER_NOT_EXIST) { 
                showTip(tr("用户不存在"), false);
            }
            else if(error == ErrorCodes::PASSWD_ERR) { 
                showTip(tr("密码错误"), false);
            }
            return;
        }
        
        // === [关键修改] 登录成功，不直接跳转，而是发起 TCP 连接 ===
        showTip(tr("登录成功，正在连接聊天服务器..."), true);
        
        // 从 GateServer 返回的 JSON 中解析 ChatServer 信息
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();
        
        // 保存 UID 和 Token，供 TCP 登录使用
        _uid = si.Uid;
        _token = si.Token;
        
        qDebug() << "Get Server Info:" << si.Host << ":" << si.Port << " Token:" << si.Token;
        
        // 发送信号给 TcpMgr，让它去连接
        emit sig_connect_tcp(si);
    });
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err) {
    if(err != ErrorCodes::SUCCESS) { showTip("网络请求错误", false); return; }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()) { showTip("JSON解析错误", false); return; }
    if(m_handlers.contains(id)) m_handlers[id](jsonDoc.object());
}

// [新增] 处理 TCP 连接结果
void LoginDialog::slot_tcp_con_finish(bool bsuccess) {
    if(bsuccess) {
        showTip(tr("服务器连接成功，正在验证..."), true);
        
        // 构造 TCP 登录包 (JSON格式)
        QJsonObject jsonObj;
        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;
        
        QJsonDocument doc(jsonObj);
        QString jsonString = doc.toJson(QJsonDocument::Indented);
        
        // 发送给 TcpMgr (ReqId 暂时复用 ID_LOGIN_USER，服务端需对应处理)
        TcpMgr::GetInstance()->slot_send_data(ReqId::ID_CHAT_LOGIN, jsonString);
        
    } else {
        showTip(tr("连接聊天服务器失败"), false);
    }
}