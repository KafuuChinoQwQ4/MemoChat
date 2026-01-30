#include "AddUserItem.h"
#include <QHBoxLayout>
#include <QLabel>

AddUserItem::AddUserItem(QWidget *parent) : ListItemBase(parent)
{
    SetItemType(ListItemType::Pp_ADD_USER_TIP_ITEM);
    initUI();
}

AddUserItem::~AddUserItem() {}

void AddUserItem::initUI()
{
    // 模拟 UI 文件布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);

    // 左侧图标
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(40, 40);
    iconLabel->setObjectName("add_tip"); // 配合 QSS 设置图片

    // 中间文字
    QLabel *textLabel = new QLabel(this);
    textLabel->setText("添加好友");
    textLabel->setObjectName("message_tip");

    // 右侧箭头
    QLabel *arrowLabel = new QLabel(this);
    arrowLabel->setFixedSize(20, 20);
    arrowLabel->setObjectName("right_tip"); // 配合 QSS

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch(); // 弹簧
    layout->addWidget(arrowLabel);
}