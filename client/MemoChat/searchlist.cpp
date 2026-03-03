#include "searchlist.h"
#include<QScrollBar>
#include "adduseritem.h"
#include "invaliditem.h"
#include "findsuccessdlg.h"
#include "tcpmgr.h"
#include "customizeedit.h"
#include "findfaildlg.h"
#include "loadingdlg.h"
#include "userdata.h"
#include "usermgr.h"

SearchList::SearchList(QWidget *parent):QListWidget(parent),_find_dlg(nullptr), _search_edit(nullptr), _send_pending(false)
{
    Q_UNUSED(parent);
     this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
     this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->viewport()->installEventFilter(this);

    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);

    addTipItem();

    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_user_search, this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDlg()
{
    if(_find_dlg){
        _find_dlg->hide();
        _find_dlg = nullptr;
    }
}

void SearchList::SetSearchEdit(QWidget* edit) {
    _search_edit = edit;
}

void SearchList::waitPending(bool pending)
{
    if(pending){
        _loadingDialog = new LoadingDlg(this);
        _loadingDialog->setModal(true);
        _loadingDialog->show();
        _send_pending = pending;
    }else{
        _loadingDialog->hide();
        _loadingDialog->deleteLater();
         _send_pending = pending;
    }
}


void SearchList::addTipItem()
{
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item_tmp->setSizeHint(QSize(250,10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);


    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
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


    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

   if(itemType == ListItemType::ADD_USER_TIP_ITEM){

       if (_send_pending) {
           return;
       }

       if (!_search_edit) {
           return;
       }
       waitPending(true);
       auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
       auto uid_str = search_edit->text();

	   QJsonObject jsonObj;
	   jsonObj["user_id"] = uid_str;

	   QJsonDocument doc(jsonObj);
	   QByteArray jsonData = doc.toJson(QJsonDocument::Compact);


       emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
       return;
   }


    CloseFindDlg();
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false);
    if (si == nullptr) {
        _find_dlg = std::make_shared<FindFailDlg>(this);
    }else{

        auto self_uid = UserMgr::GetInstance()->GetUid();
        if (si->_uid == self_uid) {
                 return;
        }


        bool bExist = UserMgr::GetInstance()->CheckFriendById(si->_uid);
        if(bExist){


            emit sig_jump_chat_item(si);
            return;
        }

        _find_dlg = std::make_shared<FindSuccessDlg>(this);
        dynamic_pointer_cast<FindSuccessDlg>(_find_dlg)->SetSearchInfo(si);

    }
    _find_dlg->show();
}
