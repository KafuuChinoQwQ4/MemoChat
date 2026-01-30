#include "GroupTipItem.h"
#include <QHBoxLayout>
#include <QLabel>

GroupTipItem::GroupTipItem(QWidget *parent) : ListItemBase(parent)
{
    SetItemType(ListItemType::Pp_GROUP_TIP_ITEM);
    initUI();
}

void GroupTipItem::initUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 0, 0, 0); // 左边留点空隙
    layout->setSpacing(0);

    _tip_lb = new QLabel(this);
    _tip_lb->setStyleSheet("color: #2e2f30; font-size: 12px; font-family: 'Microsoft YaHei';");
    _tip_lb->setFixedHeight(20);
    
    layout->addWidget(_tip_lb);
}

void GroupTipItem::SetGroupTip(QString str)
{
    _tip_lb->setText(str);
}

QSize GroupTipItem::sizeHint() const
{
    return QSize(250, 25); // 高度稍微小一点
}