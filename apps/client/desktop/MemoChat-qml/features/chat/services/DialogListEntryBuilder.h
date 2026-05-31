#ifndef DIALOGLISTENTRYBUILDER_H
#define DIALOGLISTENTRYBUILDER_H

#include <QVariantMap>
#include <memory>

#include "DialogListTypes.h"
#include "userdata.h"

class DialogListEntryBuilder
{
public:
    static std::shared_ptr<AuthInfo>
    buildPlaceholderAuthInfo(int uid, const QVariantMap& dialogItem, const QString& defaultIcon);
    static std::shared_ptr<FriendInfo> buildDialogEntry(const DialogEntrySeed& seed,
                                                        const DialogDecorationState& state);
    static QString privateDisplayName(const std::shared_ptr<FriendInfo>& friendInfo);
    static void applyFriendProfileToPrivateSeed(DialogEntrySeed& seed, const std::shared_ptr<FriendInfo>& friendInfo);
};

#endif // DIALOGLISTENTRYBUILDER_H
