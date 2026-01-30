#include "ClickedLabel.h"
#include <QMouseEvent>
#include <QDebug>

ClickedLabel::ClickedLabel(QWidget *parent):QLabel(parent), _curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickedLabel::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;
    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;
    setProperty("state",normal);
    repolish(this);
}

ClickLbState ClickedLabel::GetCurState()
{
    return _curstate;
}

bool ClickedLabel::SetCurState(ClickLbState state)
{
    _curstate = state;
    if(_curstate == ClickLbState::Normal){
        setProperty("state",_normal);
        repolish(this);
    }else if(_curstate == ClickLbState::Selected){
        setProperty("state",_selected);
        repolish(this);
    }
    return true;
}

void ClickedLabel::ResetNormalState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
}

void ClickedLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Normal){
             setProperty("state",_normal_press);
             repolish(this);
        }else{
             setProperty("state",_selected_press);
             repolish(this);
        }
        return;
    }
    QLabel::mousePressEvent(event);
}

void ClickedLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Normal){
            setProperty("state",_normal_hover);
            repolish(this);
        }else{
            setProperty("state",_selected_hover);
            repolish(this);
        }
        emit clicked(this->text(), _curstate);
        return;
    }
    QLabel::mouseReleaseEvent(event);
}

void ClickedLabel::enterEvent(QEnterEvent *event)
{
    if(_curstate == ClickLbState::Normal){
        setProperty("state",_normal_hover);
        repolish(this);
    }else{
        setProperty("state",_selected_hover);
        repolish(this);
    }
    QLabel::enterEvent(event);
}

void ClickedLabel::leaveEvent(QEvent *event)
{
    if(_curstate == ClickLbState::Normal){
        setProperty("state",_normal);
        repolish(this);
    }else{
        setProperty("state",_selected);
        repolish(this);
    }
    QLabel::leaveEvent(event);
}