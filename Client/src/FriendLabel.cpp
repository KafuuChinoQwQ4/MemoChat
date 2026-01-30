#include "FriendLabel.h"
#include <QHBoxLayout>

FriendLabel::FriendLabel(QWidget *parent) : QFrame(parent)
{
    initUI();
}

void FriendLabel::initUI()
{
    setProperty("state", "normal");
    
    // 设置水平布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(5);

    _tip_lb = new QLabel(this);
    _tip_lb->setObjectName("friend_label_tip"); 
    
    _close_lb = new ClickedLabel(this);
    _close_lb->setFixedSize(12, 12); // 假设关闭图标大小
    _close_lb->setObjectName("close_lb");
    _close_lb->SetState("normal","hover","pressed","select_normal","select_hover","select_pressed");

    connect(_close_lb, &ClickedLabel::clicked, [this](){
        emit sig_close(_text);
    });

    layout->addWidget(_tip_lb);
    layout->addWidget(_close_lb);
    
    // 自动调整大小以适应内容
    setLayout(layout);
}

void FriendLabel::SetText(QString text)
{
    _text = text;
    _tip_lb->setText(_text);
    _tip_lb->adjustSize();
    // 调整整体大小逻辑交给 Layout 自动处理，或者手动 resize
    adjustSize();
}

QString FriendLabel::Text()
{
    return _text;
}