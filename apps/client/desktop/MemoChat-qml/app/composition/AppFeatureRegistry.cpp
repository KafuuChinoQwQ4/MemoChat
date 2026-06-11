#include "AppFeatureRegistry.h"

#include "ClientGateway.h"

AppFeatureRegistry::AppFeatureRegistry(ClientGateway& gateway, QObject* parent)
    : shellViewModel(parent)
    , authController(&gateway)
    , authService(&gateway)
    , authViewModel(&authService, parent)
    , callController(&gateway, parent)
    , chatController(&gateway)
    , chatViewModel(parent)
    , chatFeatureController(chatViewModel, parent)
    , contactController(&gateway, parent)
    , groupController(parent)
    , profileController(&gateway, parent)
    , momentsController(parent)
    , agentController(&gateway)
    , petController(&gateway)
    , r18Controller(&gateway)
{
}
