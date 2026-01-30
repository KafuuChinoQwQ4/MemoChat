#pragma once
#include <QFrame>
#include <QLabel>
#include "ClickedLabel.h"

class FriendLabel : public QFrame
{
    Q_OBJECT
public:
    explicit FriendLabel(QWidget *parent = nullptr);
    void SetText(QString text);
    QString Text();
    
private:
    QLabel *_tip_lb;
    ClickedLabel *_close_lb;
    QString _text;
    void initUI();

signals:
    void sig_close(QString);
};