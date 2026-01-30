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
    // ... 逻辑同文档，略微简化 ...
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