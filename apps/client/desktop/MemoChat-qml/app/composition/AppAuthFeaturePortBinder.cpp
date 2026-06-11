#include "AppCoordinators.h"
#include "AppPortRegistry.h"

void AppPortRegistry::bindAuthFeaturePorts()
{
    _features.authViewModel.setCommandPort(AuthCommandPort{
        [this]()
        {
            clearTip();
        },
        [this](const QString& email, const QString& password)
        {
            _session_coordinator->login(email, password);
        },
        [this](const QString& email)
        {
            _session_coordinator->requestRegisterCode(email);
        },
        [this](const QString& user,
               const QString& email,
               const QString& password,
               const QString& confirm,
               const QString& verifyCode)
        {
            _session_coordinator->registerUser(user, email, password, confirm, verifyCode);
        },
        [this](const QString& email)
        {
            _session_coordinator->requestResetCode(email);
        },
        [this](const QString& user, const QString& email, const QString& password, const QString& verifyCode)
        {
            _session_coordinator->resetPassword(user, email, password, verifyCode);
        }});
}
