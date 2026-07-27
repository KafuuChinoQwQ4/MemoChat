#ifndef PETNETWORKREQUESTUTILS_H
#define PETNETWORKREQUESTUTILS_H

#include "HttpMgrRequestUtils.h"

#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtNetwork/QNetworkRequest>

namespace memochat::pet
{

inline void configurePetRequest(QNetworkRequest& request)
{
    applyBearerAccessTokenHeader(request);

    configureSecureNetworkRequest(request);
}

} // namespace memochat::pet

#endif // PETNETWORKREQUESTUTILS_H
