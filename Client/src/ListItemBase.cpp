#include "ListItemBase.h"

ListItemBase::ListItemBase(QWidget *parent) : QWidget(parent)
{
}

void ListItemBase::SetItemType(ListItemType itemType)
{
    _itemType = itemType;
}

ListItemType ListItemBase::GetItemType()
{
    return _itemType;
}