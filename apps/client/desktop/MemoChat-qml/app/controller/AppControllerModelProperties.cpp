#include "AppController.h"

FriendListModel* AppController::dialogListModel()
{
    return &_dialog_list_model;
}

FriendListModel* AppController::chatListModel()
{
    return &_chat_list_model;
}

FriendListModel* AppController::groupListModel()
{
    return &_group_list_model;
}

FriendListModel* AppController::contactListModel()
{
    return &_contact_list_model;
}

ContactController* AppController::contact()
{
    return &_contact_controller;
}

ContactController* AppController::contactController()
{
    return &_contact_controller;
}

GroupController* AppController::group()
{
    return &_group_controller;
}

GroupController* AppController::groupController()
{
    return &_group_controller;
}

ChatMessageModel* AppController::messageModel()
{
    return &_message_model;
}

SearchResultModel* AppController::searchResultModel()
{
    return &_search_result_model;
}

ApplyRequestModel* AppController::applyRequestModel()
{
    return &_apply_request_model;
}

MomentsModel* AppController::momentsModel() const
{
    return _moments_controller.model();
}

MomentsController* AppController::momentsController() const
{
    return const_cast<MomentsController*>(&_moments_controller);
}

AgentController* AppController::agentController() const
{
    return const_cast<AgentController*>(&_agent_controller);
}

AgentMessageModel* AppController::agentMessageModel() const
{
    return _agent_controller.model();
}

PetController* AppController::petController() const
{
    return const_cast<PetController*>(&_pet_controller);
}

R18Controller* AppController::r18Controller() const
{
    return const_cast<R18Controller*>(&_r18_controller);
}

ProfileController* AppController::profile()
{
    return &_profile_controller;
}

ProfileController* AppController::profileController()
{
    return &_profile_controller;
}

CallController* AppController::call()
{
    return &_call_controller;
}

CallController* AppController::callController()
{
    return &_call_controller;
}
