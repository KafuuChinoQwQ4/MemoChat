#pragma once
#include <QWidget>
#include "ListItemBase.h"

class AddUserItem : public ListItemBase
{
    Q_OBJECT
public:
    explicit AddUserItem(QWidget *parent = nullptr);
    ~AddUserItem();
    QSize sizeHint() const override {
        return QSize(250, 70); 
    }
private:
    void initUI();
};