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
#include "TcpMgr.h"
#include "UserMgr.h"
#include "ApplyFriendPage.h"

ChatDialog::ChatDialog(QWidget *parent) 
    : QDialog(parent), _b_loading(false), _state(ChatMode) 
{
    setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    initUI();
    addChatUserList();
    this->installEventFilter(this);

    //连接申请添加好友信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_friend_apply, this, &ChatDialog::slot_apply_friend);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_add_auth_friend, this, &ChatDialog::slot_add_auth_friend);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ChatDialog::slot_auth_rsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_text_chat_msg, this, &ChatDialog::slot_text_chat_msg);
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

    // [New] 联系人列表 (默认隐藏)
    _contact_list = new ContactUserList(_chat_list_wid);
    _contact_list->setObjectName("con_user_list");
    _contact_list->hide();
    connect(_contact_list, &ContactUserList::sig_switch_apply_friend_page, this, &ChatDialog::slot_switch_apply_friend_page);
    
    // [New] 搜索列表 (初始隐藏)
    _search_list = new SearchList(_chat_list_wid);
    _search_list->setObjectName("search_list");
    _search_list->hide();

    layout->addWidget(searchArea);
    layout->addWidget(_chat_list);  
    layout->addWidget(_contact_list); 
    layout->addWidget(_search_list); 
}

void ChatDialog::addChatBox() {
    _stacked_widget = new QStackedWidget(this);
    _stacked_widget->setObjectName("chat_stacked_wid"); 
    
    _chat_page = new ChatPage(this);
    QWidget *emptyPage = new QWidget(this);
    emptyPage->setStyleSheet("background-color: #f0f0f0;");

    _apply_friend_page = new ApplyFriendPage(this); // [New]
    _stacked_widget->addWidget(_apply_friend_page);
    
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
    if (bsearch) {
        _chat_list->hide();
        _contact_list->hide();
        _search_list->show();
        _state = ChatUIMode::SearchMode;
    } else {
        _search_list->hide();
        // [新增逻辑] 根据侧边栏按钮的状态决定显示哪一个列表
        // 如果联系人按钮是选中状态，就显示联系人列表
        if (_side_contact_btn->GetCurState() == ClickLbState::Selected) {
             _state = ChatUIMode::ContactMode;
             _contact_list->show();
             _chat_list->hide();
        } else {
             // 否则默认显示聊天列表
             _state = ChatUIMode::ChatMode;
             _chat_list->show();
             _contact_list->hide();
        }
    }
}

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
    
    _state = ChatUIMode::ContactMode;
    ShowSearch(false); // 现在 ShowSearch 会负责显示 _contact_list 并隐藏其他
}

void ChatDialog::slot_text_changed(const QString &str) {
    if (!str.isEmpty()) {
        ShowSearch(true);
    } else {
        ShowSearch(false);
    }
}

void ChatDialog::slot_switch_apply_friend_page() {
    qDebug() << "Switch to apply friend page";
    _stacked_widget->setCurrentWidget(_apply_friend_page);
    _contact_list->ShowRedPoint(false); // 清除红点
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply)
{
    qDebug() << "receive apply friend slot, applyuid is " << apply->_from_uid << " name is "
        << apply->_name << " desc is " << apply->_desc;

    bool b_already = UserMgr::GetInstance()->AlreadyApply(apply->_from_uid);
    if (b_already) {
        return;
    }

    UserMgr::GetInstance()->AddApplyList(apply);
    _side_contact_btn->ShowRedPoint(true);
    _contact_list->ShowRedPoint(true);
    _apply_friend_page->AddNewApply(apply);
}

void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info) {
    qDebug() << "receive slot_add_auth__friend uid is " << auth_info->_uid
        << " name is " << auth_info->_name << " nick is " << auth_info->_nick;

    //判断如果已经是好友则跳过
    auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_info->_uid);
    if (bfriend) {
        return;
    }

    UserMgr::GetInstance()->AddFriend(auth_info);

    QStringList strs = { "Hello", "Nice to meet you", "Good morning" };
    QStringList heads = { ":/res/head_1.jpg", ":/res/head_2.jpg", ":/res/head_3.jpg" };
    QStringList names = { "Alice", "Bob", "Charlie" };

    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_wid->setInfo(user_info->_name, user_info->_icon, strs[str_i]);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    _chat_list->insertItem(0, item);
    _chat_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(auth_info->_uid, item);
}

void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp) {
    qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
        << " name is " << auth_rsp->_name << " nick is " << auth_rsp->_nick;

    //判断如果已经是好友则跳过
    auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_rsp->_uid);
    if (bfriend) {
        return;
    }

    UserMgr::GetInstance()->AddFriend(auth_rsp);
    QStringList strs = { "Hello", "Nice to meet you", "Good morning" };
    QStringList heads = { ":/res/head_1.jpg", ":/res/head_2.jpg", ":/res/head_3.jpg" };
    QStringList names = { "Alice", "Bob", "Charlie" };

    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    chat_user_wid->setInfo(user_info->_name, user_info->_icon, strs[str_i]);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    _chat_list->insertItem(0, item);
    _chat_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(auth_rsp->_uid, item);
}

void ChatDialog::slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg) {
    auto find_iter = _chat_items_added.find(msg->_from_uid);
    if (find_iter != _chat_items_added.end()) {
        qDebug() << "set chat item msg, uid is " << msg->_from_uid;
        QWidget* widget = _chat_list->itemWidget(find_iter.value());
        auto chat_wid = qobject_cast<ChatUserWid*>(widget);
        if (!chat_wid) {
            return;
        }
        // chat_wid->updateLastMsg(msg->_chat_msgs);
        //更新当前聊天页面记录
        //UpdateChatMsg(msg->_chat_msgs);
        //UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);
        return;
    }

    //如果没找到，则创建新的插入listwidget

    auto* chat_user_wid = new ChatUserWid();
    //查询好友信息
    auto fi_ptr = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
    chat_user_wid->setInfo(fi_ptr->_name, fi_ptr->_icon, "");
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    // chat_user_wid->updateLastMsg(msg->_chat_msgs);
    //UserMgr::GetInstance()->AppendFriendChatMsg(msg->_from_uid, msg->_chat_msgs);
    _chat_list->insertItem(0, item);
    _chat_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(msg->_from_uid, item);
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata) {
    for (auto& msg : msgdata) {
        // 暂时只处理文本
        /*
        if (msg->_msg_content.isEmpty()) {
            continue;
        }
        
        // 判断是否是当前聊天页面
        // ...

        // 创建气泡
        auto* chat_item = new ChatItemBase(ChatRole::Other);
        chat_item->setUserName(msg->_from_uid); // 需要名字
        chat_item->setUserIcon(QPixmap(":/res/head_1.jpg")); // 需要头像
        auto* bubble = new TextBubble(ChatRole::Other, msg->_msg_content);
        chat_item->setWidget(bubble);
        
        // 添加到聊天记录列表
        // ui->chat_data_list->appendChatItem(chat_item);
        */
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

bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
       QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
       handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}

// [New] 点击逻辑判断
void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
    // 1. 判断是否处于搜索模式
    if( _state != ChatUIMode::SearchMode){ // 注意：变量名是 _state 而不是文档里的 _mode
        return;
    }

    // 2. 将鼠标点击位置转换为搜索列表坐标系中的位置
    QPoint posInSearchList = _search_list->mapFromGlobal(event->globalPos());
    
    // 3. 判断点击位置是否在搜索列表的范围内
    if (!_search_list->rect().contains(posInSearchList)) {
        // 如果不在搜索列表内，清空输入框并隐藏
        _search_edit->clear();
        ShowSearch(false);
    }
}