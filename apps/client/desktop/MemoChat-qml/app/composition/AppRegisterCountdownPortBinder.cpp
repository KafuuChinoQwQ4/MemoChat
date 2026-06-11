#include "AppCoordinators.h"
#include "AppPortRegistry.h"

RegisterCountdownPort AppPortRegistry::makeRegisterCountdownPort()
{
    return RegisterCountdownPort{[this]()
                                 {
                                     return _features.authViewModel.registerCountdown();
                                 },
                                 [this](int seconds)
                                 {
                                     _features.authViewModel.syncRegisterCountdown(seconds);
                                 },
                                 [this](bool success)
                                 {
                                     _features.authViewModel.syncRegisterSuccessPage(success);
                                 },
                                 [this]()
                                 {
                                     switchToLogin();
                                 }};
}
