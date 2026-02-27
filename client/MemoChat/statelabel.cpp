#include "statelabel.h"

StateLabel::StateLabel(QWidget* parent)
    : QLabel(parent), _curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor);
}

void StateLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (_curstate == ClickLbState::Selected) {
            qDebug() << "PressEvent, already selected:" << _selected_press;
            QLabel::mousePressEvent(event);
            return;
        }

        if (_curstate == ClickLbState::Normal) {
            qDebug() << "PressEvent, change to selected press:" << _selected_press;
            _curstate = ClickLbState::Selected;
            setProperty("state", _selected_press);
            repolish(this);
            update();
        }
        return;
    }

    QLabel::mousePressEvent(event);
}

void StateLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (_curstate == ClickLbState::Normal) {
            setProperty("state", _normal_hover);
        } else {
            setProperty("state", _selected_hover);
        }

        repolish(this);
        update();
        emit clicked();
        return;
    }

    QLabel::mousePressEvent(event);
}

void StateLabel::enterEvent(QEnterEvent* event)
{
    if (_curstate == ClickLbState::Normal) {
        setProperty("state", _normal_hover);
    } else {
        setProperty("state", _selected_hover);
    }

    repolish(this);
    update();
    QLabel::enterEvent(event);
}

void StateLabel::leaveEvent(QEvent* event)
{
    if (_curstate == ClickLbState::Normal) {
        setProperty("state", _normal);
    } else {
        setProperty("state", _selected);
    }

    repolish(this);
    update();
    QLabel::leaveEvent(event);
}

void StateLabel::SetState(QString normal, QString hover, QString press,
                          QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    setProperty("state", normal);
    repolish(this);
}

ClickLbState StateLabel::GetCurState()
{
    return _curstate;
}

void StateLabel::ClearState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state", _normal);
    repolish(this);
    update();
}

void StateLabel::SetSelected(bool bselected)
{
    if (bselected) {
        _curstate = ClickLbState::Selected;
        setProperty("state", _selected);
    } else {
        _curstate = ClickLbState::Normal;
        setProperty("state", _normal);
    }

    repolish(this);
    update();
}
