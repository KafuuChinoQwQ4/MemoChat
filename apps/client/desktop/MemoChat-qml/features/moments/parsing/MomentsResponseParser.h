#ifndef MOMENTSRESPONSEPARSER_H
#define MOMENTSRESPONSEPARSER_H

#include <QString>

class QJsonObject;

namespace memochat::moments_response
{

bool parseRootObject(const QString& response, QJsonObject* root, QString* parseError = nullptr);
int errorCode(const QJsonObject& root);

} // namespace memochat::moments_response

#endif // MOMENTSRESPONSEPARSER_H
