#ifndef CHATMESSAGEDISPATCHERGROUPPAYLOAD_H
#define CHATMESSAGEDISPATCHERGROUPPAYLOAD_H

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtGlobal>

namespace ChatMessageDispatcherGroupPayload
{
qint64 normalizeCreatedAt(qint64 createdAt);
QString compactJsonValue(const QJsonValue& value);
QString messageState(const QJsonObject& message);
QJsonObject normalizedNotifyMessage(const QJsonObject& payload);
} // namespace ChatMessageDispatcherGroupPayload

#endif // CHATMESSAGEDISPATCHERGROUPPAYLOAD_H
