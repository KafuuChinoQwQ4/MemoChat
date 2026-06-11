#include "AppController.h"

void AppController::selectGroupIndex(int index)
{
    _features.chatFeatureController.selectGroupByIndex(index);
}

void AppController::selectGroupByDialogUid(int dialogUid, qint64 groupId)
{
    _features.chatFeatureController.selectGroupByDialogUid(dialogUid, groupId);
}
