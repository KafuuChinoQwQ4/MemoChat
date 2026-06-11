#include "AppController.h"

bool AppController::hasCurrentGroup() const
{
    return _features.groupController.hasCurrentGroup();
}

int AppController::currentGroupRole() const
{
    return _features.groupController.currentGroupRole();
}

QString AppController::currentGroupName() const
{
    return _features.groupController.currentGroupName();
}

QString AppController::currentGroupCode() const
{
    return _features.groupController.currentGroupCode();
}

bool AppController::currentGroupCanChangeInfo() const
{
    return _features.groupController.currentGroupCanChangeInfo();
}

bool AppController::currentGroupCanManageAdmins() const
{
    return _features.groupController.currentGroupCanManageAdmins();
}
