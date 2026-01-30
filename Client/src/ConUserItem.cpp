#include "ConUserItem.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>

ConUserItem::ConUserItem(QWidget *parent) : ListItemBase(parent)
{
    SetItemType(ListItemType::Pp_CONTACT_USER_ITEM);
    initUI();
}

ConUserItem::~ConUserItem() {}

void ConUserItem::initUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);

    // 头像
    _icon_lb = new QLabel(this);
    _icon_lb->setFixedSize(40, 40);
    
    // 名字
    _user_name_lb = new QLabel(this);
    _user_name_lb->setObjectName("user_name_lb");
    
    // 红点 (默认隐藏)
    _red_point = new QLabel(this);
    _red_point->setFixedSize(16, 16);
    _red_point->setStyleSheet("border-image: url(:/res/red_point.png);");
    _red_point->hide();
    
    layout->addWidget(_icon_lb);
    layout->addWidget(_user_name_lb);
    layout->addStretch();
    layout->addWidget(_red_point, 0, Qt::AlignRight | Qt::AlignVCenter);
}

QSize ConUserItem::sizeHint() const
{
    return QSize(250, 70);
}

void ConUserItem::SetInfo(std::shared_ptr<AuthInfo> auth_info)
{
    _info = std::make_shared<UserInfo>(auth_info);
    QPixmap pixmap(_info->_icon);
    _icon_lb->setPixmap(pixmap.scaled(_icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    _user_name_lb->setText(_info->_name);
}

void ConUserItem::SetInfo(int uid, QString name, QString icon)
{
    _info = std::make_shared<UserInfo>(uid, name, icon);
    QPixmap pixmap(icon);
    if(pixmap.isNull()) pixmap.load(":/res/head_1.jpg"); // Fallback
    _icon_lb->setPixmap(pixmap.scaled(_icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    _user_name_lb->setText(name);
}

void ConUserItem::SetInfo(std::shared_ptr<AuthRsp> auth_rsp)
{
    _info = std::make_shared<UserInfo>(auth_rsp);
    QPixmap pixmap(_info->_icon);
    _icon_lb->setPixmap(pixmap.scaled(_icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    _user_name_lb->setText(_info->_name);
}

void ConUserItem::ShowRedPoint(bool show)
{
    _red_point->setVisible(show);
}