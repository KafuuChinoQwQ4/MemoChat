#include "ResetDialog.h"
#include "HttpMgr.h"
#include <QRegularExpression>
#include <QDebug>
#include <QJsonDocument>

ResetDialog::ResetDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    initUI();
    initHttpHandlers();

    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reset_mod_finish, 
            this, &ResetDialog::slot_reset_mod_finish);
}

ResetDialog::~ResetDialog() {}

void ResetDialog::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    m_errTip = new QLabel(this);
    m_errTip->setAlignment(Qt::AlignCenter);
    m_errTip->setProperty("state", "normal"); 

    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("用户名");

    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("邮箱");

    // 密码 + 眼睛
    QHBoxLayout *passLayout = new QHBoxLayout();
    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("新密码");
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passVisible = new ClickedLabel(this);
    m_passVisible->setFixedSize(20, 20);
    m_passVisible->SetState("unvisible", "unvisible_hover", "", "visible", "visible_hover", "");
    passLayout->addWidget(m_passEdit);
    passLayout->addWidget(m_passVisible);

    // 验证码
    QHBoxLayout *codeLayout = new QHBoxLayout();
    m_codeEdit = new QLineEdit(this);
    m_codeEdit->setPlaceholderText("验证码");
    m_getCodeBtn = new TimerBtn(this);
    m_getCodeBtn->setText("获取验证码");
    codeLayout->addWidget(m_codeEdit);
    codeLayout->addWidget(m_getCodeBtn);

    m_sureBtn = new QPushButton("确认重置", this);
    m_returnBtn = new QPushButton("返回登录", this);

    mainLayout->addWidget(m_errTip);
    mainLayout->addWidget(m_userEdit);
    mainLayout->addWidget(m_emailEdit);
    mainLayout->addLayout(passLayout);
    mainLayout->addLayout(codeLayout);
    mainLayout->addWidget(m_sureBtn);
    mainLayout->addWidget(m_returnBtn);

    // 信号连接
    connect(m_userEdit, &QLineEdit::editingFinished, this, &ResetDialog::checkUserValid);
    connect(m_emailEdit, &QLineEdit::editingFinished, this, &ResetDialog::checkEmailValid);
    connect(m_passEdit, &QLineEdit::editingFinished, this, &ResetDialog::checkPassValid);
    connect(m_codeEdit, &QLineEdit::editingFinished, this, &ResetDialog::checkVarifyValid);

    connect(m_passVisible, &ClickedLabel::clicked, this, [this](){
        if(m_passVisible->GetCurState() == ClickLbState::Normal){
            m_passEdit->setEchoMode(QLineEdit::Password);
        }else{
            m_passEdit->setEchoMode(QLineEdit::Normal);
        }
    });

    connect(m_getCodeBtn, &TimerBtn::clicked, this, &ResetDialog::onGetCodeClicked);
    connect(m_sureBtn, &QPushButton::clicked, this, &ResetDialog::onSureClicked);
    connect(m_returnBtn, &QPushButton::clicked, this, &ResetDialog::onReturnClicked);
}

void ResetDialog::showTip(QString str, bool b_ok) {
    if (b_ok) m_errTip->setProperty("state", "normal");
    else m_errTip->setProperty("state", "err");
    m_errTip->setText(str);
    repolish(m_errTip);
}

void ResetDialog::AddTipErr(TipErr te, QString tips) {
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void ResetDialog::DelTipErr(TipErr te) {
    _tip_errs.remove(te);
    if (_tip_errs.isEmpty()) showTip("", true);
    else showTip(_tip_errs.first(), false);
}

void ResetDialog::checkUserValid() {
    if (m_userEdit->text().isEmpty()) AddTipErr(TipErr::TIP_USER_ERR, "用户名不能为空");
    else DelTipErr(TipErr::TIP_USER_ERR);
}

void ResetDialog::checkEmailValid() {
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    if (!regex.match(m_emailEdit->text()).hasMatch()) AddTipErr(TipErr::TIP_EMAIL_ERR, "邮箱格式不正确");
    else DelTipErr(TipErr::TIP_EMAIL_ERR);
}

void ResetDialog::checkPassValid() {
    QRegularExpression regex("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    if (!regex.match(m_passEdit->text()).hasMatch()) AddTipErr(TipErr::TIP_PWD_ERR, "密码格式不正确");
    else DelTipErr(TipErr::TIP_PWD_ERR);
}

void ResetDialog::checkVarifyValid() {
    if (m_codeEdit->text().isEmpty()) AddTipErr(TipErr::TIP_VARIFY_ERR, "验证码不能为空");
    else DelTipErr(TipErr::TIP_VARIFY_ERR);
}

void ResetDialog::onGetCodeClicked() {
    checkEmailValid();
    if(_tip_errs.contains(TipErr::TIP_EMAIL_ERR)) return;

    QJsonObject json_obj;
    json_obj["email"] = m_emailEdit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_varifycode"),
                                        json_obj, ReqId::ID_GET_VARIFY_CODE, Modules::RESETMOD);
}

void ResetDialog::onSureClicked() {
    checkUserValid(); checkEmailValid(); checkPassValid(); checkVarifyValid();
    if (!_tip_errs.isEmpty()) return;

    QJsonObject json_obj;
    json_obj["user"] = m_userEdit->text();
    json_obj["email"] = m_emailEdit->text();
    json_obj["passwd"] = m_passEdit->text();
    json_obj["varifycode"] = m_codeEdit->text();

    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/reset_pwd"),
                 json_obj, ReqId::ID_RESET_PWD, Modules::RESETMOD);
}

void ResetDialog::onReturnClicked() {
    emit switchLogin();
}

void ResetDialog::initHttpHandlers() {
    m_handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip("参数错误", false); return;
        }
        showTip("验证码已发送", true);
    });

    m_handlers.insert(ReqId::ID_RESET_PWD, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip("重置失败", false); return;
        }
        showTip("重置成功，点击返回登录", true);
    });
}

void ResetDialog::slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err) {
    if(err != ErrorCodes::SUCCESS) { showTip("网络请求错误", false); return; }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull() || !jsonDoc.isObject()) { showTip("JSON解析错误", false); return; }
    if(m_handlers.contains(id)) m_handlers[id](jsonDoc.object());
}