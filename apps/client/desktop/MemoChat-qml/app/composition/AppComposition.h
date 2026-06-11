#ifndef APPCOMPOSITION_H
#define APPCOMPOSITION_H

#include <memory>

class AgentController;
class AgentMessageModel;
class AppController;
class AuthViewModel;
class CallController;
class ChatViewModel;
class ContactController;
class GroupController;
class MomentsController;
class MomentsModel;
class PetController;
class ProfileController;
class QQmlContext;
class R18Controller;
class ShellViewModel;

class AppComposition
{
public:
    AppComposition();
    ~AppComposition();

    AppComposition(const AppComposition&) = delete;
    AppComposition& operator=(const AppComposition&) = delete;

    ShellViewModel* shell();
    AuthViewModel* auth();
    ChatViewModel* chat();
    ContactController* contact();
    GroupController* group();
    ProfileController* profile();
    CallController* call();
    AgentController* agent();
    AgentMessageModel* agentMessages();
    MomentsController* moments();
    MomentsModel* momentsModel();
    PetController* pet();
    R18Controller* r18();
    void configureQmlContext(QQmlContext& context);

private:
    std::unique_ptr<AppController> _controller;
};

#endif // APPCOMPOSITION_H
