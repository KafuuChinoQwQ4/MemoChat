#ifndef LIVE2DMOTIONCATALOG_H
#define LIVE2DMOTIONCATALOG_H

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace Live2DMotionCatalog
{

QString actionDisplayName(const QString& name, const QString& file, const QString& fallback);
void appendActionItem(QVariantList& items,
                      QSet<QString>& actionKeys,
                      QSet<QString>& actionFiles,
                      const QString& kind,
                      const QString& name,
                      const QString& trigger,
                      const QString& group,
                      int index,
                      const QString& file,
                      const QString& source);
void appendDirectoryActionItems(QVariantList& items,
                                QSet<QString>& actionKeys,
                                QSet<QString>& actionFiles,
                                const QString& kind,
                                const QString& directoryPath,
                                const QStringList& nameFilters);

} // namespace Live2DMotionCatalog

#endif // LIVE2DMOTIONCATALOG_H
