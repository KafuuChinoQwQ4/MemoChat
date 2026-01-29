#include "ClickedLabel.h"
#include <QMouseEvent>

ClickedLabel::ClickedLabel(QWidget* parent)
    : QLabel(parent), _curstate(ClickLbState::Normal)
{
    // 设置鼠标指针为手型，提示用户可点击
    this->setCursor(Qt::PointingHandCursor);
}

// 鼠标点击事件：切换状态并刷新样式
void ClickedLabel::mousePressEvent(QMouseEvent *ev)
{
    // 仅响应左键点击
    if (ev->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Normal){
             qDebug()<<"clicked , change to selected hover: "<< _selected_hover;
             _curstate = ClickLbState::Selected;
             setProperty("state", _selected_press); 
             repolish(this);
             update();
        } else {
             qDebug()<<"clicked , change to normal hover: "<< _normal_hover;
             _curstate = ClickLbState::Normal;
             setProperty("state", _normal_press);
             repolish(this);
             update();
        }
        return;
    }
    // 调用父类处理
    QLabel::mousePressEvent(ev);
}

void ClickedLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Normal){
             // 恢复到悬停状态（因为鼠标还在上面）
             setProperty("state", _normal_hover);
             repolish(this);
             update();
        } else {
             setProperty("state", _selected_hover);
             repolish(this);
             update();
        }
        // 发送点击信号
        emit clicked();
        return;
    }
    QLabel::mouseReleaseEvent(ev);
}

// 鼠标进入事件：切换到悬停样式
void ClickedLabel::enterEvent(QEnterEvent* event)
{
    if(_curstate == ClickLbState::Normal){
         setProperty("state", _normal_hover);
    } else {
         setProperty("state", _selected_hover);
    }
    
    repolish(this);
    QLabel::enterEvent(event);
}

// 鼠标离开事件：恢复到正常样式
void ClickedLabel::leaveEvent(QEvent* event)
{
    if(_curstate == ClickLbState::Normal){
         setProperty("state", _normal);
    } else {
         setProperty("state", _selected);
    }
    
    repolish(this);
    QLabel::leaveEvent(event);
}

// 设置各种状态下的 QSS 属性值
void ClickedLabel::SetState(QString normal, QString hover, QString press,
                            QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;
    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;
    
    // 初始化为普通状态
    setProperty("state", normal);
    repolish(this);
}

ClickLbState ClickedLabel::GetCurState()
{
    return _curstate;
}