#include "ApplyFriendItem.h"
#include "ClickedBtn.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>

ApplyFriendItem::ApplyFriendItem(QWidget *parent) :
    ListItemBase(parent), _added(false)
{
    SetItemType(ListItemType::Pp_APPLY_FRIEND_ITEM);
    initUI();
}

ApplyFriendItem::~ApplyFriendItem() {}

void ApplyFriendItem::initUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);
    
    // 头像
    _icon_lb = new QLabel(this);
    _icon_lb->setFixedSize(50, 50);
    
    // 中间信息
    QWidget *infoWid = new QWidget(this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWid);
    infoLayout->setContentsMargins(0,0,0,0);
    infoLayout->setSpacing(5);
    
    _user_name_lb = new QLabel(this);
    _user_name_lb->setObjectName("user_name_lb");
    _user_chat_lb = new QLabel(this);
    _user_chat_lb->setObjectName("user_chat_lb");
    
    infoLayout->addWidget(_user_name_lb);
    infoLayout->addWidget(_user_chat_lb);
    
    // 右侧按钮
    _addBtn = new ClickedBtn(this);
    _addBtn->setObjectName("addBtn");
    _addBtn->setFixedSize(60, 30);
    _addBtn->setText("添加");
    _addBtn->SetState("normal","hover", "press");
    
    _already_add_lb = new QLabel("已添加", this);
    _already_add_lb->setObjectName("already_add_lb");
    _already_add_lb->hide();

    layout->addWidget(_icon_lb);
    layout->addWidget(infoWid);
    layout->addStretch();
    layout->addWidget(_addBtn);
    layout->addWidget(_already_add_lb);
    
    connect(_addBtn, &ClickedBtn::clicked, [this](){
        emit this->sig_auth_friend(_apply_info);
    });
}

QSize ApplyFriendItem::sizeHint() const
{
    return QSize(250, 80);
}

void ApplyFriendItem::SetInfo(std::shared_ptr<ApplyInfo> apply_info)
{
    _apply_info = apply_info;
    QPixmap pixmap(_apply_info->_icon);
    if(pixmap.isNull()) pixmap.load(":/res/head_1.jpg");
    
    _icon_lb->setPixmap(pixmap.scaled(_icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    _user_name_lb->setText(_apply_info->_name);
    _user_chat_lb->setText(_apply_info->_desc);
}

void ApplyFriendItem::ShowAddBtn(bool bshow)
{
    if (bshow) {
        _addBtn->show();
        _already_add_lb->hide();
        _added = false;
    } else {
        _addBtn->hide();
        _already_add_lb->show();
        _added = true;
    }
}

int ApplyFriendItem::GetUid() {
    return _apply_info->_uid;
}