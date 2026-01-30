#include "ChatPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QDebug>
#include "ChatView.h"       // [新增]
#include "ChatItemBase.h"   // [新增]
#include "BubbleFrame.h"    // [新增]
#include "ClickedBtn.h"
#include "ClickedLabel.h"

ChatPage::ChatPage(QWidget *parent) : QWidget(parent)
{
    initUI();
    
    // 设置按钮状态样式
    _receive_btn->SetState("normal","hover","press");
    _send_btn->SetState("normal","hover","press");
    _emo_lb->SetState("normal","hover","press","normal","hover","press");
    _file_lb->SetState("normal","hover","press","normal","hover","press");
    
    // 连接发送按钮
    connect(_send_btn, &QPushButton::clicked, this, &ChatPage::on_send_btn_clicked);
}

void ChatPage::initUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    // 1. 标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setFixedHeight(50);
    titleBar->setObjectName("chat_title_bar");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    _chat_title = new QLabel("这里是聊天对象", titleBar);
    _chat_title->setObjectName("title_lb");
    titleLayout->addWidget(_chat_title);
    titleLayout->addStretch();
    
    // 2. 消息展示区 [修改] 使用 ChatView 替换 QListWidget
    _chat_view = new ChatView(this);
    _chat_view->setObjectName("chat_view");
    
    // 3. 工具栏
    QWidget *toolBar = new QWidget(this);
    toolBar->setFixedHeight(40);
    toolBar->setObjectName("tool_wid");
    QHBoxLayout *toolLayout = new QHBoxLayout(toolBar);
    toolLayout->setContentsMargins(10, 0, 10, 0);
    toolLayout->setSpacing(10);
    _emo_lb = new ClickedLabel(toolBar);
    _emo_lb->setFixedSize(25, 25);
    _emo_lb->setObjectName("emo_lb");
    _file_lb = new ClickedLabel(toolBar);
    _file_lb->setFixedSize(25, 25);
    _file_lb->setObjectName("file_lb");
    toolLayout->addWidget(_emo_lb);
    toolLayout->addWidget(_file_lb);
    toolLayout->addStretch();

    // 4. 输入框
    _chat_edit = new QTextEdit(this);
    _chat_edit->setFixedHeight(100);
    _chat_edit->setObjectName("chatEdit");

    // 5. 底部发送栏
    QWidget *bottomBar = new QWidget(this);
    bottomBar->setFixedHeight(50);
    bottomBar->setObjectName("send_wid");
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 20, 10);
    bottomLayout->addStretch();
    _receive_btn = new ClickedBtn(bottomBar);
    _receive_btn->setText("模拟对方"); // 修改文案方便测试
    _receive_btn->setFixedSize(80, 30);
    _receive_btn->setObjectName("receive_btn");
    _send_btn = new ClickedBtn(bottomBar);
    _send_btn->setText("发送");
    _send_btn->setFixedSize(80, 30);
    _send_btn->setObjectName("send_btn");
    bottomLayout->addWidget(_receive_btn);
    bottomLayout->addWidget(_send_btn);

    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(_chat_view); // 添加 ChatView
    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(_chat_edit);
    mainLayout->addWidget(bottomBar);
}

void ChatPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatPage::on_send_btn_clicked()
{
    QString content = _chat_edit->toPlainText();
    if(content.isEmpty()) return;

    // 清空输入框
    _chat_edit->clear();

    // 创建消息条目
    ChatItemBase *pChatItem = new ChatItemBase(ChatRole::Self);
    pChatItem->setUserName("Kafuu Chino");
    pChatItem->setUserIcon(QPixmap(":/res/KafuuChino.png")); // 确保资源路径正确

    // 创建气泡 (目前只演示文本)
    QWidget *pBubble = new TextBubble(ChatRole::Self, content);
    pChatItem->setWidget(pBubble);

    // 添加到聊天列表
    _chat_view->appendChatItem(pChatItem);
}