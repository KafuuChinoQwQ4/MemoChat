#include "SearchList.h"
#include "AddUserItem.h"
#include "TcpMgr.h"
#include "LoadingDlg.h"
#include <QDebug>

SearchList::SearchList(QWidget *parent)
    : QListWidget(parent), _search_edit(nullptr), _send_pending(false), _loadingDialog(nullptr)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this);
    
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_user_search, this, &SearchList::slot_user_search);
    
    addTipItem();
}

void SearchList::addTipItem()
{
    // 1. 顶部空白间隔
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    item_tmp->setSizeHint(QSize(250, 10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);

    // 2. 添加好友提示条目
    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, add_user_item);
}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = this->itemWidget(item);
    if(!widget) return;
    
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem) return;

    if(customItem->GetItemType() == ListItemType::Pp_ADD_USER_TIP_ITEM) {
        // 点击了“添加好友”，后续这里会弹出查找用户对话框
        qDebug() << "Clicked Add User Tip Item";
        // CloseFindDlg(); // 暂时还没有 FindDlg
    }
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    // 处理搜索结果
    qDebug() << "Search User Result: " << si->_name;
}

void SearchList::SetSearchEdit(QWidget* edit) {
    _search_edit = edit;
}

void SearchList::CloseFindDlg() {
    // 关闭搜索弹窗逻辑
}

void SearchList::waitPending(bool pending) {
    if(pending) {
        _loadingDialog = new LoadingDlg(this);
        _loadingDialog->setModal(true);
        _loadingDialog->show();
        _send_pending = true;
    } else {
        _loadingDialog->hide();
        _loadingDialog->deleteLater();
        _send_pending = false;
    }
}

bool SearchList::eventFilter(QObject *watched, QEvent *event) {
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
        return true; 
    }
    return QListWidget::eventFilter(watched, event);
}