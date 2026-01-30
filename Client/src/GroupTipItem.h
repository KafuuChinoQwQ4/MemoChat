#pragma once
#include <QWidget>
#include "ListItemBase.h"

class QLabel;

class GroupTipItem : public ListItemBase
{
    Q_OBJECT
public:
    explicit GroupTipItem(QWidget *parent = nullptr);
    void SetGroupTip(QString str);
    QSize sizeHint() const override;

private:
    QLabel *_tip_lb;
    void initUI();
};