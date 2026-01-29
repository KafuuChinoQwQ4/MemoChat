#include "ChatUserWid.h"
#include <QPixmap>

ChatUserWid::ChatUserWid(QWidget *parent) : QWidget(parent)
{
    // 主布局：水平
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    mainLayout->setSpacing(10);

    // 1. 头像
    _icon_lb = new QLabel(this);
    _icon_lb->setFixedSize(45, 45);
    _icon_lb->setObjectName("icon_lb");
    
    // 2. 右侧信息区 (名字、时间、消息)
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(2);

    // 2.1 第一行：名字 + 时间
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0,0,0,0);
    
    _name_lb = new QLabel(this);
    _name_lb->setObjectName("user_name_lb");
    _name_lb->setText("User");
    
    _time_lb = new QLabel(this);
    _time_lb->setObjectName("time_lb");
    _time_lb->setText("12:00");
    
    topLayout->addWidget(_name_lb);
    topLayout->addStretch();
    topLayout->addWidget(_time_lb);

    // 2.2 第二行：聊天内容
    _user_chat_lb = new QLabel(this);
    _user_chat_lb->setObjectName("user_chat_lb");
    _user_chat_lb->setText("Chat Content...");
    // 内容过长显示省略号
    // 注意：QLabel 的省略号处理比较麻烦，这里先暂且这样，后续优化
    
    infoLayout->addLayout(topLayout);
    infoLayout->addWidget(_user_chat_lb);

    mainLayout->addWidget(_icon_lb);
    mainLayout->addLayout(infoLayout);
}

void ChatUserWid::setInfo(QString name, QString head, QString msg)
{
    _name_lb->setText(name);
    _user_chat_lb->setText(msg);
    
    // 加载头像
    QString path = ":/res/head_" + head + ".png"; // 假设你有 head_1.png 等资源
    // 为了防止没有资源导致空白，我们先用默认头像
    QPixmap pix(":/res/KafuuChino.png"); 
    // QPixmap pix(path); 
    
    // 图片缩放
    _icon_lb->setPixmap(pix.scaled(_icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    // 时间暂时写死或生成随机
    _time_lb->setText("13:24"); 
}