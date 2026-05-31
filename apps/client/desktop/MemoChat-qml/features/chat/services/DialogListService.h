#ifndef DIALOGLISTSERVICE_H
#define DIALOGLISTSERVICE_H

#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QVariantMap>
#include <memory>
#include <vector>
#include "DialogListTypes.h"
#include "userdata.h"

class DialogListService
{
public:
    static std::shared_ptr<AuthInfo>
    buildPlaceholderAuthInfo(int uid, const QVariantMap& dialogItem, const QString& defaultIcon);
    static std::shared_ptr<FriendInfo> buildDialogEntry(const DialogEntrySeed& seed,
                                                        const DialogDecorationState& state);
    static QString privateDisplayName(const std::shared_ptr<FriendInfo>& friendInfo);
    static void applyFriendProfileToPrivateSeed(DialogEntrySeed& seed, const std::shared_ptr<FriendInfo>& friendInfo);
    static void appendMissingPrivateDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs,
                                            const std::vector<std::shared_ptr<FriendInfo>>& friends,
                                            const QSet<int>& existingPrivateUids,
                                            const DialogDecorationState& state);
    static void appendMissingGroupDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs,
                                          const std::vector<std::shared_ptr<GroupInfoData>>& groups,
                                          QMap<int, qint64>& groupUidMap,
                                          const QSet<qint64>& existingGroupIds,
                                          const DialogDecorationState& state);
    static void appendExistingDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs,
                                      const std::vector<QVariantMap>& existingDialogs,
                                      const DialogDecorationState& state);
    static void sortDialogs(std::vector<std::shared_ptr<FriendInfo>>& dialogs);
};

#endif // DIALOGLISTSERVICE_H
