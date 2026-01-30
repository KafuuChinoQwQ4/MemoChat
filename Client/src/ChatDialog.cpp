#include "ChatDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>
#include "CustomizeEdit.h"
#include "ChatUserWid.h"
#include <QStackedWidget>

ChatDialog::ChatDialog(QWidget *parent) 
    : QDialog(parent), _b_loading(false), _state(ChatMode) 
{
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    initUI();
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
    mainLayout->addWidget(_stacked_widget); 
}

void ChatDialog::addSideBar() {
    _side_bar = new QWidget(this);
    // [修改 1] 侧边栏宽度变窄：70 -> 46
    _side_bar->setFixedWidth(46); 
    _side_bar->setObjectName("side_bar"); 

    QVBoxLayout *layout = new QVBoxLayout(_side_bar);
    // [修改 2] 调整边距以保持图标居中：左右边距设为 8 (46 - 30) / 2 = 8
    layout->setContentsMargins(8, 30, 8, 0);
    layout->setSpacing(20);

    QPushButton *avatar = new QPushButton(_side_bar);
    // [修改 3] 头像缩小：50 -> 30
    avatar->setFixedSize(30, 30);
    avatar->setObjectName("side_avatar");

    _side_chat_btn = new StateWidget(_side_bar);
    // [修改 4] 聊天图标缩小：50 -> 30
    _side_chat_btn->setFixedSize(30, 30);
    _side_chat_btn->setObjectName("side_chat_lb");
    _side_chat_btn->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    _side_chat_btn->SetSelected(true); 

    _side_contact_btn = new StateWidget(_side_bar);
    // [修改 5] 联系人图标缩小：50 -> 30
    _side_contact_btn->setFixedSize(30, 30);
    _side_contact_btn->setObjectName("side_contact_lb");
    _side_contact_btn->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");

    AddLBGroup(_side_chat_btn);
    AddLBGroup(_side_contact_btn);

    connect(_side_chat_btn, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(_side_contact_btn, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

    layout->addWidget(avatar);
    layout->addWidget(_side_chat_btn);
    layout->addWidget(_side_contact_btn);
    layout->addStretch();
}

void ChatDialog::addChatList() {
    _chat_list_wid = new QWidget(this);
    _chat_list_wid->setFixedWidth(250);
    _chat_list_wid->setObjectName("chat_list_wid");

    QVBoxLayout *layout = new QVBoxLayout(_chat_list_wid);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(10);

    // 搜索区
    QWidget *searchArea = new QWidget(_chat_list_wid);
    searchArea->setFixedHeight(40); 
    QHBoxLayout *searchLayout = new QHBoxLayout(searchArea);
    searchLayout->setContentsMargins(10, 0, 10, 0);
    searchLayout->setSpacing(10);

    QWidget *search_back = new QWidget(searchArea);
    search_back->setObjectName("search_back"); 
    search_back->setFixedHeight(30); 

    QHBoxLayout *search_back_layout = new QHBoxLayout(search_back);
    search_back_layout->setContentsMargins(10, 0, 0, 0); 
    search_back_layout->setSpacing(5);

    QLabel *search_icon = new QLabel(search_back);
    search_icon->setFixedSize(15, 15);
    QPixmap searchPix(":/res/search.png"); 
    search_icon->setPixmap(searchPix.scaled(15, 15, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    search_icon->setAttribute(Qt::WA_TranslucentBackground);

    _search_edit = new CustomizeEdit(search_back);
    _search_edit->setObjectName("search_edit"); 
    _search_edit->setPlaceholderText("搜索");
    _search_edit->setMaxLength(20);
    
    // [New] 连接文本变化信号
    connect(_search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);

    search_back_layout->addWidget(search_icon);
    search_back_layout->addWidget(_search_edit);

    // 这里的 AddBtn 暂时保留 ClickedBtn 或者也改成 StateWidget，这里暂不改动
    QPushButton *addBtn = new QPushButton(searchArea); 
    addBtn->setObjectName("add_btn");
    addBtn->setFixedSize(25, 25);

    searchLayout->addWidget(search_back);
    searchLayout->addWidget(addBtn);

    // === [Modified] 列表区域 ===
    // 聊天列表
    _chat_list = new ChatUserList(_chat_list_wid);
    _chat_list->setObjectName("chat_list");
    connect(_chat_list, &ChatUserList::sig_loading_chat_user, this, &ChatDialog::slot_loading_chat_user);
    
    // [New] 搜索列表 (初始隐藏)
    _search_list = new SearchList(_chat_list_wid);
    _search_list->setObjectName("search_list");
    _search_list->hide();

    layout->addWidget(searchArea);
    layout->addWidget(_chat_list);   // 两个列表都加入布局
    layout->addWidget(_search_list); 
}

void ChatDialog::addChatBox() {
    _stacked_widget = new QStackedWidget(this);
    _stacked_widget->setObjectName("chat_stacked_wid"); 
    
    _chat_page = new ChatPage(this);
    QWidget *emptyPage = new QWidget(this);
    emptyPage->setStyleSheet("background-color: #f0f0f0;");
    
    _stacked_widget->addWidget(emptyPage); 
    _stacked_widget->addWidget(_chat_page); 
    _stacked_widget->setCurrentWidget(_chat_page);
}

// [New] 辅助函数实现
void ChatDialog::AddLBGroup(StateWidget *lb) {
    _lb_list.push_back(lb);
}

void ChatDialog::ClearLabelState(StateWidget *lb) {
    for(auto & ele: _lb_list){
        if(ele == lb) continue;
        ele->ClearState();
    }
}

void ChatDialog::ShowSearch(bool bsearch) {
    if(bsearch){
        _chat_list->hide();
        _search_list->show();
        _state = ChatUIMode::SearchMode;
    } else {
        _chat_list->show();
        _search_list->hide();
        _state = ChatUIMode::ChatMode;
    }
}

// [New] 槽函数实现
void ChatDialog::slot_side_chat() {
    qDebug()<< "receive side chat clicked";
    ClearLabelState(_side_chat_btn);
    _stacked_widget->setCurrentWidget(_chat_page);
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void ChatDialog::slot_side_contact() {
    qDebug()<< "receive side contact clicked";
    ClearLabelState(_side_contact_btn);
    // 这里暂时切换到空白页，后续可以做联系人页面
    _stacked_widget->setCurrentWidget(_stacked_widget->widget(0)); 
    _state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slot_text_changed(const QString &str) {
    if (!str.isEmpty()) {
        ShowSearch(true);
    } else {
        ShowSearch(false);
    }
}

// ... existing slot_loading_chat_user & addChatUserList ...
void ChatDialog::slot_loading_chat_user() {
    // 保持原有逻辑
    if(_b_loading) return;
    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    QTimer::singleShot(1000, [this, loadingDialog](){
        addChatUserList();
        loadingDialog->deleteLater();
        _b_loading = false;
    });
}

void ChatDialog::addChatUserList() {
    // 保持原有逻辑
    QStringList names = {"Kafuu Chino", "Cocoa", "Rize"};
    QStringList msgs = {"Hello!", "Where are you?", "Coffee?"};
    for(int i = 0; i < 3; i++){
        ChatUserWid *chat_user_wid = new ChatUserWid();
        chat_user_wid->setInfo(names[i], "1", msgs[i]);
        QListWidgetItem *item = new QListWidgetItem(_chat_list);
        item->setSizeHint(chat_user_wid->sizeHint()); 
        _chat_list->setItemWidget(item, chat_user_wid); 
    }
}