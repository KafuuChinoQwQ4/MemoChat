#ifndef MEMOCHAT_TEMPLATE_AUTHSTATE_H
#define MEMOCHAT_TEMPLATE_AUTHSTATE_H

#include <QString>

struct AuthState
{
    QString tipText;
    bool tipError = false;
    bool busy = false;
    bool registerSuccessPage = false;
    int registerCountdown = 0;
};

#endif // MEMOCHAT_TEMPLATE_AUTHSTATE_H
