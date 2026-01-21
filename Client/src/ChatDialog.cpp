#include "ChatDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QDebug>
#include "ClickedBtn.h"

ChatDialog::ChatDialog(QWidget *parent) : QDialog(parent) {
    // 设置无边框，根据 Day18 文档
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    resize(900, 700); // 默认大小
    initUI();
}

ChatDialog::~ChatDialog() {}

void ChatDialog::initUI() {
    // 1. 主布局 (水平: 侧边栏 | 列表 | 聊天框)
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    addSideBar();
    addChatList();
    addChatBox();

    mainLayout->addWidget(_side_bar);
    mainLayout->addWidget(_chat_list_wid);
    mainLayout->addWidget(_chat_box_wid);
}

void ChatDialog::addSideBar() {
    _side_bar = new QWidget(this);
    _side_bar->setFixedWidth(70); // 侧边栏固定宽度
    _side_bar->setObjectName("side_bar"); // 用于QSS样式

    QVBoxLayout *layout = new QVBoxLayout(_side_bar);
    layout->setContentsMargins(10, 30, 10, 0);
    layout->setSpacing(20);

    // 头像 (这里暂时用 Button 模拟)
    QPushButton *avatar = new QPushButton(_side_bar);
    avatar->setFixedSize(50, 50);
    avatar->setObjectName("side_avatar");

    // 聊天图标
    ClickedBtn *chatBtn = new ClickedBtn(_side_bar);
    chatBtn->setFixedSize(50, 50);
    chatBtn->setObjectName("side_chat_btn");
    chatBtn->SetState("normal", "hover", "press");

    // 联系人图标
    ClickedBtn *contactBtn = new ClickedBtn(_side_bar);
    contactBtn->setFixedSize(50, 50);
    contactBtn->setObjectName("side_contact_btn");
    contactBtn->SetState("normal", "hover", "press");

    layout->addWidget(avatar);
    layout->addWidget(chatBtn);
    layout->addWidget(contactBtn);
    layout->addStretch(); // 底部弹簧
}

void ChatDialog::addChatList() {
    _chat_list_wid = new QWidget(this);
    _chat_list_wid->setFixedWidth(250); // 中间栏宽度
    _chat_list_wid->setObjectName("chat_list_wid");

    QVBoxLayout *layout = new QVBoxLayout(_chat_list_wid);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(10);

    // 搜索框区域
    QWidget *searchArea = new QWidget(_chat_list_wid);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchArea);
    searchLayout->setContentsMargins(10, 0, 10, 0);
    
    _search_edit = new QLineEdit(searchArea);
    _search_edit->setPlaceholderText("搜索");
    _search_edit->setFixedHeight(30);

    QPushButton *addBtn = new QPushButton(searchArea);
    addBtn->setFixedSize(30, 30);
    addBtn->setText("+"); // 暂时用文字

    searchLayout->addWidget(_search_edit);
    searchLayout->addWidget(addBtn);

    // 聊天列表
    _chat_list = new QListWidget(_chat_list_wid);
    _chat_list->setObjectName("chat_list");

    layout->addWidget(searchArea);
    layout->addWidget(_chat_list);
}

void ChatDialog::addChatBox() {
    _chat_box_wid = new QWidget(this);
    _chat_box_wid->setObjectName("chat_box_wid");

    QVBoxLayout *layout = new QVBoxLayout(_chat_box_wid);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 1. 标题栏
    QWidget *titleBar = new QWidget(_chat_box_wid);
    titleBar->setFixedHeight(50);
    titleBar->setObjectName("chat_title_bar");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    _chat_title = new QLabel("这里是聊天对象", titleBar);
    titleLayout->addWidget(_chat_title);
    titleLayout->addStretch();

    // 2. 消息显示区
    _msg_show_list = new QListWidget(_chat_box_wid);
    _msg_show_list->setObjectName("msg_list");

    // 3. 工具栏 (表情、文件等)
    QWidget *toolBar = new QWidget(_chat_box_wid);
    toolBar->setFixedHeight(30);
    QHBoxLayout *toolLayout = new QHBoxLayout(toolBar);
    toolLayout->setContentsMargins(10, 0, 0, 0);
    // 示例按钮
    QPushButton *emojiBtn = new QPushButton("😊", toolBar);
    emojiBtn->setFixedSize(25, 25);
    emojiBtn->setFlat(true);
    toolLayout->addWidget(emojiBtn);
    toolLayout->addStretch();

    // 4. 输入区
    _chat_edit = new QTextEdit(_chat_box_wid);
    _chat_edit->setFixedHeight(100);

    // 5. 发送按钮区
    QWidget *bottomBar = new QWidget(_chat_box_wid);
    bottomBar->setFixedHeight(50);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 20, 10);
    bottomLayout->addStretch();
    
    QPushButton *sendBtn = new QPushButton("发送", bottomBar);
    sendBtn->setFixedSize(80, 30);
    bottomLayout->addWidget(sendBtn);

    // 添加到主布局
    layout->addWidget(titleBar);
    layout->addWidget(_msg_show_list);
    layout->addWidget(toolBar);
    layout->addWidget(_chat_edit);
    layout->addWidget(bottomBar);
}