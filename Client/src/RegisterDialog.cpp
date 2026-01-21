#include "RegisterDialog.h"
#include "HttpMgr.h"
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

RegisterDialog::RegisterDialog(QWidget *parent) : QDialog(parent), m_countdown(5) {
    // 设置无边框窗口
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    
    // 初始化 UI 和 HTTP 处理函数
    initUI();
    initHttpHandlers();

    // 连接 HttpMgr 的注册模块信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, 
            this, &RegisterDialog::slot_reg_mod_finish);
}

RegisterDialog::~RegisterDialog() {
    // 析构函数，如有需要手动释放的资源写在这里
    // Qt 的对象树机制会自动释放子控件
}

void RegisterDialog::initUI() {
    // 1. 初始化 StackedLayout 用于管理“注册页”和“成功提示页”
    m_stackedLayout = new QStackedLayout(this);
    
    // === 页面1: 注册填写页 ===
    m_pageRegister = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageRegister);
    
    // 错误提示 Label
    m_errTip = new QLabel(this);
    m_errTip->setObjectName("m_errTip"); // 配合 QSS 设置样式
    m_errTip->setAlignment(Qt::AlignCenter);
    m_errTip->setProperty("state", "normal"); 

    // 输入框：用户名
    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("用户名");

    // 输入框：邮箱
    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("邮箱");

    // 输入框：密码 + 眼睛图标布局
    QHBoxLayout *passLayout = new QHBoxLayout();
    m_passEdit = new QLineEdit(this);
    m_passEdit->setPlaceholderText("密码");
    m_passEdit->setEchoMode(QLineEdit::Password);
    
    m_passVisible = new ClickedLabel(this);
    m_passVisible->setObjectName("pass_visible");
    m_passVisible->setFixedSize(20, 20); // 根据图标大小调整
    // 设置初始状态 (需要在 stylesheet.qss 中定义 visible/unvisible 样式)
    m_passVisible->SetState("unvisible", "unvisible_hover", "", "visible", "visible_hover", "");
    
    passLayout->addWidget(m_passEdit);
    passLayout->addWidget(m_passVisible);

    // 输入框：确认密码 + 眼睛图标布局
    QHBoxLayout *confirmLayout = new QHBoxLayout();
    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setPlaceholderText("确认密码");
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    
    m_confirmVisible = new ClickedLabel(this);
    m_confirmVisible->setObjectName("confirm_visible");
    m_confirmVisible->setFixedSize(20, 20);
    m_confirmVisible->SetState("unvisible", "unvisible_hover", "", "visible", "visible_hover", "");
    
    confirmLayout->addWidget(m_confirmEdit);
    confirmLayout->addWidget(m_confirmVisible);

    // 验证码区域 (使用 TimerBtn)
    QHBoxLayout *codeLayout = new QHBoxLayout();
    m_codeEdit = new QLineEdit(this);
    m_codeEdit->setPlaceholderText("验证码");
    
    m_getCodeBtn = new TimerBtn(this); // 使用自定义倒计时按钮
    m_getCodeBtn->setText("获取验证码");
    
    codeLayout->addWidget(m_codeEdit);
    codeLayout->addWidget(m_getCodeBtn);

    // 底部按钮
    m_registerBtn = new QPushButton("注册", this);
    m_cancelBtn = new QPushButton("取消", this);

    // 添加控件到主布局 (注意顺序)
    mainLayout->addWidget(m_errTip);
    mainLayout->addWidget(m_userEdit);
    mainLayout->addWidget(m_emailEdit);
    mainLayout->addLayout(passLayout);    // 放入密码布局
    mainLayout->addLayout(confirmLayout); // 放入确认密码布局
    mainLayout->addLayout(codeLayout);
    mainLayout->addWidget(m_registerBtn);
    mainLayout->addWidget(m_cancelBtn);

    // === 页面2: 注册成功页 ===
    m_pageSuccess = new QWidget(this);
    QVBoxLayout *successLayout = new QVBoxLayout(m_pageSuccess);
    m_successTip = new QLabel(this);
    m_successTip->setAlignment(Qt::AlignCenter);
    // 设置字体大一点
    QFont font = m_successTip->font();
    font.setPointSize(12);
    m_successTip->setFont(font);
    
    successLayout->addStretch();
    successLayout->addWidget(m_successTip);
    successLayout->addStretch();
    
    // 将两个页面加入 StackedLayout
    m_stackedLayout->addWidget(m_pageRegister);
    m_stackedLayout->addWidget(m_pageSuccess);
    m_stackedLayout->setCurrentWidget(m_pageRegister); // 默认显示注册页

    // === 信号连接 ===
    
    // 1. 输入校验 (失去焦点时触发)
    connect(m_userEdit, &QLineEdit::editingFinished, this, &RegisterDialog::checkUserValid);
    connect(m_emailEdit, &QLineEdit::editingFinished, this, &RegisterDialog::checkEmailValid);
    connect(m_passEdit, &QLineEdit::editingFinished, this, &RegisterDialog::checkPassValid);
    connect(m_confirmEdit, &QLineEdit::editingFinished, this, &RegisterDialog::checkConfirmValid);
    connect(m_codeEdit, &QLineEdit::editingFinished, this, &RegisterDialog::checkVarifyValid);

    // 2. 眼睛图标点击逻辑 (切换明文/密文)
    connect(m_passVisible, &ClickedLabel::clicked, this, [this]() {
        if (m_passVisible->GetCurState() == ClickLbState::Normal) {
            m_passEdit->setEchoMode(QLineEdit::Password);
        } else {
            m_passEdit->setEchoMode(QLineEdit::Normal);
        }
    });
    connect(m_confirmVisible, &ClickedLabel::clicked, this, [this]() {
        if (m_confirmVisible->GetCurState() == ClickLbState::Normal) {
            m_confirmEdit->setEchoMode(QLineEdit::Password);
        } else {
            m_confirmEdit->setEchoMode(QLineEdit::Normal);
        }
    });

    // 3. 按钮逻辑
    connect(m_getCodeBtn, &TimerBtn::clicked, this, &RegisterDialog::onGetCodeClicked); // 注意这里 TimerBtn 的信号可能是 clicked()
    connect(m_cancelBtn, &QPushButton::clicked, this, &RegisterDialog::switchLogin);
    connect(m_registerBtn, &QPushButton::clicked, this, &RegisterDialog::onRegisterClicked);
    
    // 4. 定时器初始化 (用于成功跳转)
    m_countdownTimer = new QTimer(this);
    connect(m_countdownTimer, &QTimer::timeout, [this](){
        if(m_countdown == 0){
            m_countdownTimer->stop();
            emit switchLogin(); // 倒计时结束，切回登录
            return;
        }
        m_successTip->setText(QString("注册成功，%1 秒后返回登录").arg(m_countdown));
        m_countdown--;
    });
}

// 刷新错误提示 Label 的样式
void RegisterDialog::showTip(QString str, bool b_ok) {
    if (b_ok) {
        m_errTip->setProperty("state", "normal");
    } else {
        m_errTip->setProperty("state", "err");
    }
    m_errTip->setText(str);
    repolish(m_errTip); // 刷新样式
}

// === 错误管理逻辑 ===
void RegisterDialog::AddTipErr(TipErr te, QString tips) {
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void RegisterDialog::DelTipErr(TipErr te) {
    _tip_errs.remove(te);
    if (_tip_errs.isEmpty()) {
        showTip("", true);
    } else {
        // 显示优先级最高的错误（Map 自动按 Key 排序，TipErr 枚举值越小优先级越高）
        showTip(_tip_errs.first(), false);
    }
}

// === 校验逻辑 ===
void RegisterDialog::checkUserValid() {
    if (m_userEdit->text().isEmpty()) {
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
}

void RegisterDialog::checkEmailValid() {
    auto email = m_emailEdit->text();
    // 邮箱正则验证
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if (!match) {
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
}

void RegisterDialog::checkPassValid() {
    auto pass = m_passEdit->text();
    // 密码正则：6-15位，包含字母数字
    QRegularExpression regex("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regex.match(pass).hasMatch();
    if (!match) {
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6-15位"));
        return;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    
    // 密码变了，确认密码也要重新校验一下
    checkConfirmValid();
}

void RegisterDialog::checkConfirmValid() {
    if (m_confirmEdit->text() != m_passEdit->text()) {
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("两次密码不一致"));
        return;
    }
    DelTipErr(TipErr::TIP_CONFIRM_ERR);
}

void RegisterDialog::checkVarifyValid() {
    if (m_codeEdit->text().isEmpty()) {
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return;
    }
    DelTipErr(TipErr::TIP_VARIFY_ERR);
}

// === 槽函数 ===

void RegisterDialog::onGetCodeClicked()
{
    // 获取验证码前，先校验邮箱格式
    checkEmailValid();
    if(_tip_errs.contains(TipErr::TIP_EMAIL_ERR)){
        return;
    }

    auto email = m_emailEdit->text();
    QJsonObject json_obj;
    json_obj["email"] = email;

    // 发送 POST 请求
    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/get_varifycode"),
        json_obj, 
        ReqId::ID_GET_VARIFY_CODE, 
        Modules::REGISTERMOD
    );
}

void RegisterDialog::onRegisterClicked() {
    // 1. 点击注册时，强制执行一遍所有检查
    checkUserValid();
    checkEmailValid();
    checkPassValid();
    checkConfirmValid();
    checkVarifyValid();

    // 2. 如果 Map 中有错误，说明校验没通过，直接返回
    if (!_tip_errs.isEmpty()) {
        return; 
    }

    // 3. 校验通过，发送 HTTP 请求
    QJsonObject json_obj;
    json_obj["user"] = m_userEdit->text();
    json_obj["email"] = m_emailEdit->text();
    json_obj["passwd"] = m_passEdit->text();
    json_obj["confirm"] = m_confirmEdit->text();
    json_obj["varifycode"] = m_codeEdit->text();

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_register"),
        json_obj, 
        ReqId::ID_REG_USER, 
        Modules::REGISTERMOD
    );
}

void RegisterDialog::initHttpHandlers() {
    // 1. 处理“获取验证码”回包
    m_handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip("参数错误", false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip("验证码已发送到邮箱", true);
        qDebug() << "Email sent to:" << email;
    });

    // 2. 处理“注册用户”回包
    m_handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("注册失败"), false);
            return;
        }
        
        // === 注册成功逻辑 ===
        showTip(tr("注册成功"), true);
        
        // 切换到成功页面
        m_stackedLayout->setCurrentWidget(m_pageSuccess);
        
        // 启动倒计时 (5秒)
        m_countdown = 5;
        m_successTip->setText(QString("注册成功，%1 秒后返回登录").arg(m_countdown));
        m_countdownTimer->start(1000); 
    });
}

// 统一的 HTTP 完成槽函数，分发到 m_handlers
void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err) {
    if (err != ErrorCodes::SUCCESS) {
        showTip("网络请求错误", false);
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        showTip("JSON 解析错误", false);
        return;
    }

    if (m_handlers.contains(id)) {
        m_handlers[id](jsonDoc.object());
    }
}