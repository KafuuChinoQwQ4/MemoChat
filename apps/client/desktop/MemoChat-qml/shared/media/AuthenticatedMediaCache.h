#ifndef AUTHENTICATEDMEDIACACHE_H
#define AUTHENTICATEDMEDIACACHE_H

#include <QString>

namespace memochat::media
{

QString resolveAuthenticatedMediaDownloadUrl(const QString& remoteUrl, const QString& accessToken);

} // namespace memochat::media

#endif // AUTHENTICATEDMEDIACACHE_H
