#ifndef PROFILEREQUESTPAYLOADS_H
#define PROFILEREQUESTPAYLOADS_H

#include <QJsonObject>
#include <QString>

namespace memochat::profile_payload
{
bool validateProfile(const QString& nick, const QString& desc, QString* errorText);
QJsonObject buildSaveProfilePayload(const QString& name, const QString& nick, const QString& desc, const QString& icon);
} // namespace memochat::profile_payload

#endif // PROFILEREQUESTPAYLOADS_H
