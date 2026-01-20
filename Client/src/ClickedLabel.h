#pragma once
#include <QLabel>
#include "global.h"

class ClickedLabel : public QLabel
{
    Q_OBJECT
public:
    ClickedLabel(QWidget* parent = nullptr);
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void enterEvent(QEnterEvent* event) override; // Qt6 use QEnterEvent
    virtual void leaveEvent(QEvent* event) override;
    
    void SetState(QString normal="", QString hover="", QString press="",
                  QString select="", QString select_hover="", QString select_press="");
    ClickLbState GetCurState();

signals:
    void clicked();

private:
    QString _normal;
    QString _normal_hover;
    QString _normal_press;
    QString _selected;
    QString _selected_hover;
    QString _selected_press;
    ClickLbState _curstate;
};