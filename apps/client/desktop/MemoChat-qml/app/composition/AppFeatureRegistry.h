#ifndef APPFEATUREREGISTRY_H
#define APPFEATUREREGISTRY_H

#include "AgentController.h"
#include "AuthController.h"
#include "AuthService.h"
#include "AuthViewModel.h"
#include "CallController.h"
#include "CallSessionModel.h"
#include "ChatController.h"
#include "ChatFeatureController.h"
#include "ChatViewModel.h"
#include "ContactController.h"
#include "GroupController.h"
#include "ApplyRequestModel.h"
#include "FriendListModel.h"
#include "SearchResultModel.h"
#include "MomentsController.h"
#include "MomentsModel.h"
#include "PetController.h"
#include "ProfileController.h"
#include "R18Controller.h"
#include "AgentMessageModel.h"
#include "LivekitBridge.h"
#include "ShellViewModel.h"

class ClientGateway;
class QObject;

struct AppFeatureRegistry
{
    AppFeatureRegistry(ClientGateway& gateway, QObject* parent);

    ShellViewModel shellViewModel;
    AuthController authController;
    AuthService authService;
    AuthViewModel authViewModel;
    CallController callController;
    ChatController chatController;
    ChatViewModel chatViewModel;
    ChatFeatureController chatFeatureController;
    ContactController contactController;
    GroupController groupController;
    ProfileController profileController;
    MomentsController momentsController;
    AgentController agentController;
    PetController petController;
    R18Controller r18Controller;
};

#endif // APPFEATUREREGISTRY_H
