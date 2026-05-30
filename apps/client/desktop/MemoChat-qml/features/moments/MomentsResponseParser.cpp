#include "MomentsResponseParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace memochat::moments_response
{

bool parseRootObject(const QString& response, QJsonObject* root, QString* parseError)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
    {
        if (parseError)
        {
            *parseError = error.errorString();
        }
        if (root)
        {
            *root = QJsonObject();
        }
        return false;
    }
    if (root)
    {
        *root = doc.object();
    }
    if (parseError)
    {
        parseError->clear();
    }
    return true;
}

int errorCode(const QJsonObject& root)
{
    return root.value(QStringLiteral("error")).toInt();
}

} // namespace memochat::moments_response
