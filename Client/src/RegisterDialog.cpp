#include "RegisterDialog.h"
#include "HttpMgr.h"
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

RegisterDialog::RegisterDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    initUI();
    initHttpHandlers();

    // 连接 HttpMgr 的信号到本界面的槽
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, 
            this, &RegisterDialog::slot_reg_mod_finish);
}

void RegisterDialog::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 错误提示 Label
    m_errTip = new QLabel(this);
    m_errTip->setObjectName("m_errTip");
    m_errTip->setAlignment(Qt::AlignCenter);
    m_errTip->setProperty("state", "normal"); // 配合 QSS
    
    // 输入框
    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("用户名");
    
    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("邮箱");
    
    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("密码");
    m_passEdit->setEchoMode(QLineEdit::Password);
    
    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setPlaceholderText("确认密码");
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    
    // 验证码区域
    QHBoxLayout *codeLayout = new QHBoxLayout();
    m_codeEdit = new QLineEdit(this);
    m_codeEdit->setPlaceholderText("验证码");
    m_getCodeBtn = new QPushButton("获取验证码", this);
    codeLayout->addWidget(m_codeEdit);
    codeLayout->addWidget(m_getCodeBtn);

    // 底部按钮
    m_registerBtn = new QPushButton("注册", this);
    m_cancelBtn = new QPushButton("取消", this);

    // 布局组装
    mainLayout->addWidget(m_errTip);
    mainLayout->addWidget(m_userEdit);
    mainLayout->addWidget(m_emailEdit);
    mainLayout->addWidget(m_passEdit);
    mainLayout->addWidget(m_confirmEdit);
    mainLayout->addLayout(codeLayout);
    mainLayout->addWidget(m_registerBtn);
    mainLayout->addWidget(m_cancelBtn);

    // 信号连接
    connect(m_getCodeBtn, &QPushButton::clicked, this, &RegisterDialog::onGetCodeClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &RegisterDialog::switchLogin);
}

void RegisterDialog::showTip(QString str, bool b_ok) {
    if (b_ok) {
        m_errTip->setProperty("state", "normal");
    } else {
        m_errTip->setProperty("state", "err");
    }
    m_errTip->setText(str);
    repolish(m_errTip); // 刷新样式
}

void RegisterDialog::onGetCodeClicked()
{
    auto email = m_emailEdit->text();
    // 邮箱正则验证
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();

    if(match){
        // 构造 JSON 对象
        QJsonObject json_obj;
        json_obj["email"] = email;

        // 发送 POST 请求 (使用读取到的 gate_url_prefix)
        HttpMgr::GetInstance()->PostHttpReq(
            QUrl(gate_url_prefix + "/get_varifycode"),
            json_obj, 
            ReqId::ID_GET_VARIFY_CODE, 
            Modules::REGISTERMOD
        );
    } else {
        showTip(tr("邮箱地址不正确"), false);
    }
}

void RegisterDialog::initHttpHandlers() {
    // 注册：处理“获取验证码”的回包
    m_handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip("参数错误", false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip("验证码已发送到邮箱: " + email, true);
        qDebug() << "Email sent to:" << email;
    });
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err) {
    if (err != ErrorCodes::SUCCESS) {
        showTip("网络请求错误", false);
        return;
    }

    // 解析 JSON
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        showTip("JSON 解析错误", false);
        return;
    }

    // 回调处理
    if (m_handlers.contains(id)) {
        m_handlers[id](jsonDoc.object());
    }
}