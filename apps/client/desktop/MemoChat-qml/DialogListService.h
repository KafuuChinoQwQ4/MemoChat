#ifndef DIALOGLISTSERVICE_H
#define DIALOGLISTSERVICE_H

#include <QHash>
#include <QJsonObject>
#include <QSet>
#include <QVariantMap>
#include <memory>
#include <vector>
#include "userdata.h"

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
    const QHash<int, QString> *draftMap = nullptr;
    const QHash<int, int> *mentionMap = nullptr;
    const QHash<int, int> *serverPinnedMap = nullptr;
    const QHash<int, int> *serverMuteMap = nullptr;
    const QSet<int> *localPinnedSet = nullptr;
};

class DialogListService
{
public:
    static std::shared_ptr<AuthInfo> buildPlaceholderAuthInfo(int uid,
                                                              const QVariantMap &dialogItem,
                                                              const QString &defaultIcon);
    static std::shared_ptr<FriendInfo> buildDialogEntry(const DialogEntrySeed &seed,
                                                        const DialogDecorationState &state);
    static void sortDialogs(std::vector<std::shared_ptr<FriendInfo>> &dialogs);
};

#endif // DIALOGLISTSERVICE_H
