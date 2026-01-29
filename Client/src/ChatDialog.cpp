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
#include <QStackedWidget> // [关键] 必须包含
#include <QTimer>
#include "CustomizeEdit.h"
#include <QPixmap>

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
    addChatBox(); // 初始化右侧区域

    mainLayout->addWidget(_side_bar);
    mainLayout->addWidget(_chat_list_wid);
    
    // [修改] 添加 stacked widget 到布局
    mainLayout->addWidget(_stacked_widget); 
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

    // === 搜索区域 ===
    QWidget *searchArea = new QWidget(_chat_list_wid);
    searchArea->setFixedHeight(40); // 限制搜索区整体高度
    QHBoxLayout *searchLayout = new QHBoxLayout(searchArea);
    searchLayout->setContentsMargins(10, 0, 10, 0);
    searchLayout->setSpacing(10); // 搜索框和添加按钮之间的间距

    // 1. 搜索框容器 (用于绘制圆角灰色背景)
    QWidget *search_back = new QWidget(searchArea);
    search_back->setObjectName("search_back"); // 用于QSS样式
    search_back->setFixedHeight(30); // 搜索框扁一点

    QHBoxLayout *search_back_layout = new QHBoxLayout(search_back);
    search_back_layout->setContentsMargins(10, 0, 0, 0); // 左边距留给图标
    search_back_layout->setSpacing(5);

    // 1.1 放大镜图标
    QLabel *search_icon = new QLabel(search_back);
    search_icon->setFixedSize(15, 15);
    // 请确保 qrc 中有 search.png，否则这里不显示或显示空白
    QPixmap searchPix(":/res/search.png"); 
    search_icon->setPixmap(searchPix.scaled(15, 15, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    search_icon->setAttribute(Qt::WA_TranslucentBackground);

    // 1.2 输入框 (CustomizeEdit)
    _search_edit = new CustomizeEdit(search_back);
    _search_edit->setObjectName("search_edit"); // 用于QSS去边框
    _search_edit->setPlaceholderText("搜索");
    _search_edit->setMaxLength(20);

    search_back_layout->addWidget(search_icon);
    search_back_layout->addWidget(_search_edit);

    // 2. 添加好友按钮 (使用 ClickedBtn)
    ClickedBtn *addBtn = new ClickedBtn(searchArea);
    addBtn->setObjectName("add_btn");
    addBtn->setFixedSize(25, 25);
    // 设置三态 (需要QSS中对应图片)
    addBtn->SetState("normal", "hover", "press");

    // 将控件加入搜索布局
    searchLayout->addWidget(search_back);
    searchLayout->addWidget(addBtn);

    // === 列表区域 ===
    _chat_list = new ChatUserList(_chat_list_wid);
    _chat_list->setObjectName("chat_list");
    connect(_chat_list, &ChatUserList::sig_loading_chat_user, 
            this, &ChatDialog::slot_loading_chat_user);

    layout->addWidget(searchArea);
    layout->addWidget(_chat_list);
}

void ChatDialog::addChatBox() {
    // [修改] 使用 QStackedWidget 和 ChatPage
    _stacked_widget = new QStackedWidget(this);
    _stacked_widget->setObjectName("chat_stacked_wid"); // 也可以设置样式
    
    // 创建聊天页面
    _chat_page = new ChatPage(this);
    
    // 创建一个空的默认页面 (可选)
    QWidget *emptyPage = new QWidget(this);
    emptyPage->setStyleSheet("background-color: #f0f0f0;");
    
    _stacked_widget->addWidget(emptyPage); // Index 0
    _stacked_widget->addWidget(_chat_page); // Index 1
    
    // 默认显示聊天页
    _stacked_widget->setCurrentWidget(_chat_page);
}

void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading){
        return;
    }

    _b_loading = true;
    
    // 显示加载框
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show(); // 非模态显示，或者使用 exec() 模态显示
    
    qDebug() << "add new data to list.....";
    
    // 模拟耗时操作，实际上这里应该去请求网络
    // 这里我们用 QTimer 模拟一下延时，否则太快了看不见效果
    QTimer::singleShot(1000, [this, loadingDialog](){
        addChatUserList(); // 添加模拟数据
        loadingDialog->deleteLater(); // 关闭加载框
        _b_loading = false;
    });
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