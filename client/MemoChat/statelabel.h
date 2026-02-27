#ifndef STATELABEL_H
#define STATELABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QEnterEvent>

#include "global.h"

class StateLabel : public QLabel
{
    Q_OBJECT
public:
    explicit StateLabel(QWidget* parent = nullptr);

    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

    void SetState(QString normal = "", QString hover = "", QString press = "",
                  QString select = "", QString select_hover = "", QString select_press = "");

    ClickLbState GetCurState();
    void ClearState();
    void SetSelected(bool bselected);

private:
    QString _normal;
    QString _normal_hover;
    QString _normal_press;

    QString _selected;
    QString _selected_hover;
    QString _selected_press;

    ClickLbState _curstate;

signals:
    void clicked();
};

#endif // STATELABEL_H
