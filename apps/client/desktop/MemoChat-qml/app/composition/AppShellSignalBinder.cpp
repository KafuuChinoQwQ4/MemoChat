#include "AppController.h"

void AppController::bindAppShellSignals()
{
    connect(&_features.shellViewModel, &ShellViewModel::switchToLoginRequested, this, &AppController::switchToLogin);
    connect(&_features.shellViewModel,
            &ShellViewModel::switchToRegisterRequested,
            this,
            &AppController::switchToRegister);
    connect(&_features.shellViewModel, &ShellViewModel::switchToResetRequested, this, &AppController::switchToReset);
    connect(&_features.shellViewModel, &ShellViewModel::switchChatTabRequested, this, &AppController::switchChatTab);
    connect(&_features.shellViewModel,
            &ShellViewModel::beginPostLoginBootstrapRequested,
            this,
            &AppController::beginPostLoginBootstrap);
    connect(&_features.shellViewModel,
            &ShellViewModel::openExternalResourceRequested,
            this,
            &AppController::openExternalResource);
}
