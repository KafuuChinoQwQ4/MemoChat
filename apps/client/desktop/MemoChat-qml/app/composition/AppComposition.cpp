#include "AppComposition.h"

#include "AppController.h"

#include <QQmlContext>

AppComposition::AppComposition()
    : _controller(std::make_unique<AppController>())
{
}

AppComposition::~AppComposition() = default;

ShellViewModel* AppComposition::shell()
{
    return _controller->shellViewModel();
}

AuthViewModel* AppComposition::auth()
{
    return _controller->authViewModel();
}

ChatViewModel* AppComposition::chat()
{
    return _controller->chatViewModel();
}

ContactController* AppComposition::contact()
{
    return _controller->contactController();
}

GroupController* AppComposition::group()
{
    return _controller->groupController();
}

ProfileController* AppComposition::profile()
{
    return _controller->profileController();
}

CallController* AppComposition::call()
{
    return _controller->callController();
}

AgentController* AppComposition::agent()
{
    return _controller->agentController();
}

AgentMessageModel* AppComposition::agentMessages()
{
    return _controller->agentMessageModel();
}

MomentsController* AppComposition::moments()
{
    return _controller->momentsController();
}

MomentsModel* AppComposition::momentsModel()
{
    return _controller->momentsModel();
}

PetController* AppComposition::pet()
{
    return _controller->petController();
}

R18Controller* AppComposition::r18()
{
    return _controller->r18Controller();
}

void AppComposition::configureQmlContext(QQmlContext& context)
{
    context.setContextProperty("shell", shell());
    context.setContextProperty("auth", auth());
    context.setContextProperty("chat", chat());
    context.setContextProperty("contact", contact());
    context.setContextProperty("group", group());
    context.setContextProperty("profile", profile());
    context.setContextProperty("call", call());
    context.setContextProperty("agent", agent());
    context.setContextProperty("agentMessages", agentMessages());
    context.setContextProperty("moments", moments());
    context.setContextProperty("momentsModel", momentsModel());
    context.setContextProperty("pet", pet());
    context.setContextProperty("r18", r18());
}
