#include "SearchList.h"
#include "AddUserItem.h"
#include "TcpMgr.h"
#include "LoadingDlg.h"
#include "FindSuccessDlg.h"
#include "ListItemBase.h"
#include "CustomizeEdit.h" // [新增] 需要引入 Edit 头文件以便获取文本
#include <QJsonObject>     // [新增]
#include <QJsonDocument>   // [新增]
#include <QDebug>
#include "FindFailDlg.h"
#include "ApplyFriend.h"

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
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    
    // [修改] 这里加上 Pp_ 前缀，与 global.h 保持一致
    if(itemType == ListItemType::Pp_INVALID_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    // [修改] 这里也要确认是 Pp_ADD_USER_TIP_ITEM
    if(itemType == ListItemType::Pp_ADD_USER_TIP_ITEM){ 
        
        if(_send_pending){
            return;
        }

        if (!_search_edit) {
            return;
        }

        // 显示加载框
        waitPending(true);

        // 获取搜索框内容
        auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        if(!search_edit) {
            qDebug() << "Search edit cast failed";
            waitPending(false);
            return;
        }
        
        auto uid_str = search_edit->text();

        // 构建 JSON 请求
        QJsonObject jsonObj;
        jsonObj["uid"] = uid_str;

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 发送请求
        // 注意：这里使用了 ReqId::ID_SEARCH_USER_REQ，请确保在 global.h 中添加了它
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
        return;
    }

    // 清除弹出框 (如果有逻辑的话)
    CloseFindDlg();
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false); // 关闭加载动画

    if(si == nullptr){
        // 搜索失败或未找到
        _find_dlg = std::make_shared<FindFailDlg>(this);
    }else{
        // 搜索成功
        // [Todo] 此处未来可以增加逻辑判断是否已经是好友
        auto dlg = std::make_shared<FindSuccessDlg>(this);
        dlg->SetSearchInfo(si);
        _find_dlg = dlg;
    }

    _find_dlg->show();
}

void SearchList::SetSearchEdit(QWidget* edit) {
    _search_edit = edit;
}

void SearchList::CloseFindDlg() {
    if(_find_dlg){
        _find_dlg->hide();
    }
}

void SearchList::waitPending(bool pending) {
    if(pending) {
        if(_loadingDialog == nullptr){
            _loadingDialog = new LoadingDlg(this);
        }
        _loadingDialog->setModal(true);
        _loadingDialog->show();
        _send_pending = true;
    } else {
        if(_loadingDialog){
            _loadingDialog->hide();
            // 注意：deleteLater 交给 Qt 事件循环处理，避免重复创建开销也可以不 delete，看你需求
            // 这里为了匹配你给的代码，我们选择 deleteLater
             _loadingDialog->deleteLater(); 
             _loadingDialog = nullptr;
        }
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