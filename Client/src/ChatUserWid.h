#pragma once
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "global.h"

class ChatUserWid : public QWidget
{
    Q_OBJECT
public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    
    // 设置必须的信息
    void setInfo(QString name, QString head, QString msg);
    
    // 供外部获取大小，用于 QListWidgetItem
    QSize sizeHint() const override { return QSize(250, 70); } 

private:
    QLabel *_name_lb;
    QLabel *_user_chat_lb;
    QLabel *_time_lb;
    QLabel *_icon_lb; // 头像
};