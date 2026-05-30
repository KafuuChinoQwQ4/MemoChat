#ifndef MOMENTSENTRYPARSER_H
#define MOMENTSENTRYPARSER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

#include "MomentsModel.h"

MomentItem parseMomentItem(const QJsonObject& obj);
MomentLike parseMomentLike(const QJsonObject& obj);
MomentComment parseMomentComment(const QJsonObject& obj);
QVector<MomentComment> parseMomentComments(const QJsonArray& arr);
MomentEntry parseMomentEntryFromJson(const QJsonObject& obj);

#endif // MOMENTSENTRYPARSER_H
