#include "ContactUserList.h"
#include "ConUserItem.h"
#include "GroupTipItem.h"
#include <QRandomGenerator>
#include <QDebug>
#include "TcpMgr.h"
#include "UserMgr.h"

ContactUserList::ContactUserList(QWidget *parent) : QListWidget(parent)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this);

    addContactUserList();
    connect(this, &QListWidget::itemClicked, this, &ContactUserList::slot_item_clicked);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp, this, &ContactUserList::slot_auth_rsp);
}

void ContactUserList::ShowRedPoint(bool bshow)
{
    _add_friend_item->ShowRedPoint(bshow);
}

void ContactUserList::addContactUserList()
{
    // 1. 分组：新的朋友
    auto * groupTip = new GroupTipItem();
    groupTip->SetGroupTip("新的朋友");
    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(groupTip->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, groupTip);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);

    // 2. 新的朋友条目
    _add_friend_item = new ConUserItem();
    _add_friend_item->setObjectName("new_friend_item");
    _add_friend_item->SetInfo(0, tr("新的朋友"), ":/res/add_friend.png");
    _add_friend_item->SetItemType(ListItemType::Pp_APPLY_FRIEND_ITEM); // 注意枚举前缀

    QListWidgetItem *add_item = new QListWidgetItem;
    add_item->setSizeHint(_add_friend_item->sizeHint());
    this->addItem(add_item);
    this->setItemWidget(add_item, _add_friend_item);

    // 3. 分组：联系人
    auto * groupCon = new GroupTipItem();
    groupCon->SetGroupTip(tr("联系人"));
    _groupitem = new QListWidgetItem;
    _groupitem->setSizeHint(groupCon->sizeHint());
    this->addItem(_groupitem);
    this->setItemWidget(_groupitem, groupCon);
    _groupitem->setFlags(_groupitem->flags() & ~Qt::ItemIsSelectable);

    // 4. 模拟联系人数据
    QStringList names = {"Alice", "Bob", "Chino", "David", "Eva"};
    QStringList heads = {":/res/head_1.jpg", ":/res/head_2.jpg", ":/res/head_3.jpg"};

    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100);
        int head_i = randomValue % heads.size();
        int name_i = randomValue % names.size();

        auto *con_user_wid = new ConUserItem();
        con_user_wid->SetInfo(0, names[name_i], heads[head_i]);
        QListWidgetItem *item = new QListWidgetItem;
        item->setSizeHint(con_user_wid->sizeHint());
        this->addItem(item);
        this->setItemWidget(item, con_user_wid);
    }
}

bool ContactUserList::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
        
        // 模拟加载更多
        QScrollBar *scrollBar = this->verticalScrollBar();
        if (scrollBar->maximum() - scrollBar->value() <= 0) {
            emit sig_loading_contact_user();
        }
        return true; 
    }
    return QListWidget::eventFilter(watched, event);
}

void ContactUserList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = this->itemWidget(item); 
    if(!widget) return;

    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem) return;

    auto itemType = customItem->GetItemType();
    
    if(itemType == ListItemType::Pp_APPLY_FRIEND_ITEM){
       emit sig_switch_apply_friend_page();
       return;
    }

    if(itemType == ListItemType::Pp_CONTACT_USER_ITEM){
       emit sig_switch_friend_info_page();
       return;
    }
}

void ContactUserList::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    qDebug() << "slot auth rsp called";
    bool isFriend = UserMgr::GetInstance()->CheckFriendById(auth_rsp->_uid);
    if (isFriend) {
        return;
    }
    // 在 groupitem 之后插入新项
    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    QStringList heads = { ":/res/head_1.jpg", ":/res/head_2.jpg", ":/res/head_3.jpg" };
    int head_i = randomValue % heads.size();

    auto* con_user_wid = new ConUserItem();
    con_user_wid->SetInfo(auth_rsp->_uid, auth_rsp->_name, heads[head_i]);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(con_user_wid->sizeHint());

    // 获取 groupitem 的索引
    int index = this->row(_groupitem);
    // 在 groupitem 之后插入新项
    this->insertItem(index + 1, item);

    this->setItemWidget(item, con_user_wid);
}