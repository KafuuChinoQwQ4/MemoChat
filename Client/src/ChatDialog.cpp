#include "ChatDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QTextEdit>
#include <QDebug>
#include <QRandomGenerator> // 用于生成随机数据
#include "ClickedBtn.h"
#include "ChatUserWid.h" // [新增]

ChatDialog::ChatDialog(QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    initUI();
    
    // [新增] 初始化完成后加载数据
    addChatUserList();
}

ChatDialog::~ChatDialog() {}

void ChatDialog::initUI() {
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

// ... addSideBar 保持不变 ...
void ChatDialog::addSideBar() {
    _side_bar = new QWidget(this);
    _side_bar->setFixedWidth(70); 
    _side_bar->setObjectName("side_bar"); 

    QVBoxLayout *layout = new QVBoxLayout(_side_bar);
    layout->setContentsMargins(10, 30, 10, 0);
    layout->setSpacing(20);

    QPushButton *avatar = new QPushButton(_side_bar);
    avatar->setFixedSize(50, 50);
    avatar->setObjectName("side_avatar");

    ClickedBtn *chatBtn = new ClickedBtn(_side_bar);
    chatBtn->setFixedSize(50, 50);
    chatBtn->setObjectName("side_chat_btn");
    chatBtn->SetState("normal", "hover", "press");

    ClickedBtn *contactBtn = new ClickedBtn(_side_bar);
    contactBtn->setFixedSize(50, 50);
    contactBtn->setObjectName("side_contact_btn");
    contactBtn->SetState("normal", "hover", "press");

    layout->addWidget(avatar);
    layout->addWidget(chatBtn);
    layout->addWidget(contactBtn);
    layout->addStretch();
}


void ChatDialog::addChatList() {
    _chat_list_wid = new QWidget(this);
    _chat_list_wid->setFixedWidth(250); 
    _chat_list_wid->setObjectName("chat_list_wid");

    QVBoxLayout *layout = new QVBoxLayout(_chat_list_wid);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(10);

    QWidget *searchArea = new QWidget(_chat_list_wid);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchArea);
    searchLayout->setContentsMargins(10, 0, 10, 0);
    
    // [修改] 使用 CustomizeEdit
    _search_edit = new CustomizeEdit(searchArea);
    _search_edit->setPlaceholderText("搜索");
    _search_edit->setFixedHeight(30);
    _search_edit->SetMaxLength(20);

    QPushButton *addBtn = new QPushButton(searchArea);
    addBtn->setFixedSize(30, 30);
    addBtn->setText("+"); 

    searchLayout->addWidget(_search_edit);
    searchLayout->addWidget(addBtn);

    _chat_list = new QListWidget(_chat_list_wid);
    _chat_list->setObjectName("chat_list");
    // [新增] 隐藏滚动条但保留功能 (可选)
    _chat_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _chat_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    layout->addWidget(searchArea);
    layout->addWidget(_chat_list);
}

// ... addChatBox 保持不变 ...
void ChatDialog::addChatBox() {
    _chat_box_wid = new QWidget(this);
    _chat_box_wid->setObjectName("chat_box_wid");

    QVBoxLayout *layout = new QVBoxLayout(_chat_box_wid);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget *titleBar = new QWidget(_chat_box_wid);
    titleBar->setFixedHeight(50);
    titleBar->setObjectName("chat_title_bar");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    _chat_title = new QLabel("这里是聊天对象", titleBar);
    titleLayout->addWidget(_chat_title);
    titleLayout->addStretch();

    _msg_show_list = new QListWidget(_chat_box_wid);
    _msg_show_list->setObjectName("msg_list");

    QWidget *toolBar = new QWidget(_chat_box_wid);
    toolBar->setFixedHeight(30);
    QHBoxLayout *toolLayout = new QHBoxLayout(toolBar);
    toolLayout->setContentsMargins(10, 0, 0, 0);
    QPushButton *emojiBtn = new QPushButton("😊", toolBar);
    emojiBtn->setFixedSize(25, 25);
    emojiBtn->setFlat(true);
    toolLayout->addWidget(emojiBtn);
    toolLayout->addStretch();

    _chat_edit = new QTextEdit(_chat_box_wid);
    _chat_edit->setFixedHeight(100);

    QWidget *bottomBar = new QWidget(_chat_box_wid);
    bottomBar->setFixedHeight(50);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 20, 10);
    bottomLayout->addStretch();
    
    QPushButton *sendBtn = new QPushButton("发送", bottomBar);
    sendBtn->setFixedSize(80, 30);
    bottomLayout->addWidget(sendBtn);

    layout->addWidget(titleBar);
    layout->addWidget(_msg_show_list);
    layout->addWidget(toolBar);
    layout->addWidget(_chat_edit);
    layout->addWidget(bottomBar);
}

// [新增] 填充测试数据
void ChatDialog::addChatUserList()
{
    // 模拟数据
    QStringList names = {"Kafuu Chino", "Cocoa", "Rize", "Chiya", "Syaro", "Maya", "Megu", "Aoyama", "Tippy"};
    QStringList msgs = {"Hello!", "Where are you?", "Coffee?", "Good Morning", "Coding...", "Test Message", "[Image]", "See you later", "Bye"};
    
    for(int i = 0; i < 13; i++){
        // 随机取数据
        int name_i = QRandomGenerator::global()->bounded(names.size());
        int msg_i = QRandomGenerator::global()->bounded(msgs.size());

        ChatUserWid *chat_user_wid = new ChatUserWid();
        chat_user_wid->setInfo(names[name_i], "1", msgs[msg_i]); // 头像暂时都传 "1"

        QListWidgetItem *item = new QListWidgetItem(_chat_list);
        item->setSizeHint(chat_user_wid->sizeHint()); // 设置 Item 大小
        _chat_list->setItemWidget(item, chat_user_wid); // 将 Widget 放入 Item
    }
}