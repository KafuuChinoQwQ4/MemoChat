#ifndef DIALOGLISTTYPES_H
#define DIALOGLISTTYPES_H

#include <QHash>
#include <QSet>
#include <QString>

struct DialogEntrySeed
{
    int dialogUid = 0;
    QString dialogType;
    QString userId;
    QString name;
    QString nick;
    QString icon;
    QString desc;
    QString back;
    QString previewText;
    int sex = 0;
    int unreadCount = 0;
    int pinnedRank = 0;
    QString draftText;
    qint64 lastMsgTs = 0;
    int muteState = 0;
};

struct DialogDecorationState
{
    const QHash<int, QString>* draftMap = nullptr;
    const QHash<int, int>* mentionMap = nullptr;
    const QHash<int, int>* serverPinnedMap = nullptr;
    const QHash<int, int>* serverMuteMap = nullptr;
    const QSet<int>* localPinnedSet = nullptr;
};

#endif // DIALOGLISTTYPES_H
