#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include "global.h"

class ChatItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ChatItemBase(ChatRole role, QWidget *parent = nullptr);
    void setUserName(const QString &name);
    void setUserIcon(const QPixmap &icon);
    void setWidget(QWidget *w); // 放入具体的 TextBubble 或 PictureBubble

private:
    ChatRole m_role;
    QLabel *m_pNameLabel;
    QLabel *m_pIconLabel;
    QWidget *m_pBubble; // 占位或实际内容
};