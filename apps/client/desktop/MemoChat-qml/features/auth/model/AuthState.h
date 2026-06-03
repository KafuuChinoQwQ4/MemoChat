#ifndef AUTHSTATE_H
#define AUTHSTATE_H

#include <QString>

struct AuthState
{
    QString tipText;
    bool tipError = false;
    bool busy = false;
    bool registerSuccessPage = false;
    int registerCountdown = 5;
    int registerCodeCooldownSeconds = 0;
    bool registerCodeRequestPending = false;
};

#endif // AUTHSTATE_H
