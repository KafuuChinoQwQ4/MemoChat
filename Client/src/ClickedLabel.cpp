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
             _curstate = ClickLbState::Selected;
             // 点击后鼠标肯定还在控件上，所以设置为选中悬停状态
             setProperty("state", _selected_hover); 
        } else {
             _curstate = ClickLbState::Normal;
             // 同理，设置为普通悬停状态
             setProperty("state", _normal_hover);
        }
        
        // 刷新 QSS 样式 (repolish 在 global.h 中定义)
        repolish(this);
        
        // 发送点击信号，供外部槽函数调用
        emit clicked();
    }
    
    // 调用父类处理（保持默认行为）
    QLabel::mousePressEvent(ev);
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