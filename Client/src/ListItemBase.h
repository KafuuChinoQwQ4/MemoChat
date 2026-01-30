#pragma once
#include <QWidget>
#include "global.h"

class ListItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget *parent = nullptr);
    void SetItemType(ListItemType itemType);
    ListItemType GetItemType();

private:
    ListItemType _itemType;
};