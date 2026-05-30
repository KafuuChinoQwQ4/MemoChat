#ifndef LOCALFILEPICKERSERVICEPRIVATE_H
#define LOCALFILEPICKERSERVICEPRIVATE_H

#include <QString>
#include <QVariantMap>

QString inferMomentAttachmentType(const QString& filename);
QVariantMap buildAttachmentMap(const QString& filename, const QString& type);

#endif // LOCALFILEPICKERSERVICEPRIVATE_H
