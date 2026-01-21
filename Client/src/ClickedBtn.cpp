#include "ClickedBtn.h"
#include <QVariant>
#include <QStyle>

ClickedBtn::ClickedBtn(QWidget *parent) : QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor); // 鼠标变成小手
    setFlat(true); // 扁平化，去掉默认边框
}

ClickedBtn::~ClickedBtn() {}

void ClickedBtn::SetState(QString normal, QString hover, QString press)
{
    _normal = normal;
    _hover = hover;
    _press = press;
    
    setProperty("state", normal);
    style()->unpolish(this);
    style()->polish(this);
}

void ClickedBtn::enterEvent(QEnterEvent *event)
{
    setProperty("state", _hover);
    style()->unpolish(this);
    style()->polish(this);
    QPushButton::enterEvent(event);
}

void ClickedBtn::leaveEvent(QEvent *event)
{
    setProperty("state", _normal);
    style()->unpolish(this);
    style()->polish(this);
    QPushButton::leaveEvent(event);
}

void ClickedBtn::mousePressEvent(QMouseEvent *event)
{
    setProperty("state", _press);
    style()->unpolish(this);
    style()->polish(this);
    QPushButton::mousePressEvent(event);
}

void ClickedBtn::mouseReleaseEvent(QMouseEvent *event)
{
    setProperty("state", _hover);
    style()->unpolish(this);
    style()->polish(this);
    QPushButton::mouseReleaseEvent(event);
}