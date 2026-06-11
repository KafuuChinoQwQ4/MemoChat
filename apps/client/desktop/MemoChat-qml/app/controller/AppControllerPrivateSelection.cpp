#include "AppController.h"

void AppController::selectChatIndex(int index)
{
    _features.chatFeatureController.selectPrivateByIndex(index);
}

void AppController::selectChatByUid(int uid)
{
    _features.chatFeatureController.selectPrivateByUid(uid);
}
