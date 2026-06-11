#include "AppController.h"

ContactController* AppController::contactController()
{
    return &_features.contactController;
}

GroupController* AppController::groupController()
{
    return &_features.groupController;
}

MomentsModel* AppController::momentsModel() const
{
    return _features.momentsController.model();
}

MomentsController* AppController::momentsController() const
{
    return const_cast<MomentsController*>(&_features.momentsController);
}

AgentController* AppController::agentController() const
{
    return const_cast<AgentController*>(&_features.agentController);
}

AgentMessageModel* AppController::agentMessageModel() const
{
    return _features.agentController.model();
}

PetController* AppController::petController() const
{
    return const_cast<PetController*>(&_features.petController);
}

R18Controller* AppController::r18Controller() const
{
    return const_cast<R18Controller*>(&_features.r18Controller);
}

ProfileController* AppController::profileController()
{
    return &_features.profileController;
}

CallController* AppController::callController()
{
    return &_features.callController;
}
