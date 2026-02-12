#include "ApplyFriendPage.h"
#include "ApplyFriendList.h"
#include "ApplyFriendItem.h"
#include "UserMgr.h"
#include "TcpMgr.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyleOption>
#include <QRandomGenerator>
#include <QDebug>
#include "AuthenFriend.h"

ApplyFriendPage::ApplyFriendPage(QWidget *parent) : QWidget(parent)
{
    initUI();
    loadApplyList();
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ApplyFriendPage::slot_auth_rsp);
}

ApplyFriendPage::~ApplyFriendPage() {}

void ApplyFriendPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    
    // 标题栏
    QWidget *titleWid = new QWidget(this);
    titleWid->setFixedHeight(60);
    titleWid->setObjectName("friend_apply_wid");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleWid);
    titleLayout->setContentsMargins(20,0,0,0);
    
    QLabel *label = new QLabel("新的朋友", titleWid);
    label->setObjectName("friend_apply_lb");
    titleLayout->addWidget(label);
    
    // 列表
    _apply_friend_list = new ApplyFriendList(this);
    connect(_apply_friend_list, &ApplyFriendList::sig_show_search, this, &ApplyFriendPage::sig_show_search);
    
    mainLayout->addWidget(titleWid);
    mainLayout->addWidget(_apply_friend_list);
}

void ApplyFriendPage::loadApplyList()
{
    // 模拟数据
    QStringList names = {"Jack", "Pony", "Mark"};
    QStringList descs = {"Hello", "I am Pony", "Meta"};
    
    for(int i=0; i<3; i++){
        auto *apply_item = new ApplyFriendItem();
        auto apply = std::make_shared<ApplyInfo>(0, names[i], descs[i],
                                    ":/res/head_1.jpg", names[i], 0, 1);
        apply_item->SetInfo(apply);
        apply_item->ShowAddBtn(true);
        
        QListWidgetItem *item = new QListWidgetItem;
        item->setSizeHint(apply_item->sizeHint());
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
        _apply_friend_list->addItem(item);
        _apply_friend_list->setItemWidget(item, apply_item);
        
        connect(apply_item, &ApplyFriendItem::sig_auth_friend, [](std::shared_ptr<ApplyInfo> info){
             qDebug() << "Auth friend: " << info->_name;
        });
    }
}

void ApplyFriendPage::AddNewApply(std::shared_ptr<AddFriendApply> apply)
{
    //先模拟头像随机，以后头像资源增加资源服务器后再显示
    QStringList heads = { ":/res/head_1.jpg", ":/res/head_2.jpg", ":/res/head_3.jpg" };
    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int head_i = randomValue % heads.size();
    auto* apply_item = new ApplyFriendItem();
    auto apply_info = std::make_shared<ApplyInfo>(apply->_from_uid,
        apply->_name, apply->_desc, heads[head_i], apply->_name, 0, 0);
    apply_item->SetInfo(apply_info);
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(apply_item->sizeHint());
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled & ~Qt::ItemIsSelectable);
    _apply_friend_list->insertItem(0, item);
    _apply_friend_list->setItemWidget(item, apply_item);
    apply_item->ShowAddBtn(true);
    //收到审核好友信号
    connect(apply_item, &ApplyFriendItem::sig_auth_friend, [this](std::shared_ptr<ApplyInfo> apply_info) {
        auto* authFriend = new AuthenFriend(this);
        authFriend->setModal(true);
        authFriend->SetApplyInfo(apply_info);
        authFriend->show();
        });
}

void ApplyFriendPage::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto uid = auth_rsp->_uid;
    auto find_iter = _unauth_items.find(uid);
    if (find_iter == _unauth_items.end()) {
        return;
    }
    find_iter->second->ShowAddBtn(false);
}

void ApplyFriendPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}