#include "AppController.h"

namespace
{
constexpr qint64 kDefaultAdminPermBits = (1LL << 0) | (1LL << 1) | (1LL << 2) | (1LL << 4) | (1LL << 5);
constexpr qint64 kOwnerPermBits = kDefaultAdminPermBits | (1LL << 3) | (1LL << 6);
} // namespace

void AppController::setCurrentGroup(qint64 groupId, const QString& name, const QString& groupCode)
{
    const QString normalizedName = (groupId > 0) ? name : QString();
    const QString normalizedCode = (groupId > 0) ? groupCode : QString();
    auto groupInfo = groupId > 0 ? _gateway.userMgr()->GetGroupById(groupId) : nullptr;
    int role = groupInfo ? groupInfo->_role : 0;
    qint64 permissionBits = 0;
    if (groupInfo)
    {
        permissionBits = groupInfo->_permission_bits;
        if (role >= 3)
        {
            permissionBits = kOwnerPermBits;
        }
        else if (role == 2 && permissionBits <= 0)
        {
            permissionBits = kDefaultAdminPermBits;
        }
    }
    if (currentGroupId() == groupId && currentGroupName() == normalizedName && currentGroupCode() == normalizedCode &&
        currentGroupRole() == role && currentGroupPermissionBitsRaw() == permissionBits)
    {
        return;
    }

    _features.groupController.setCurrentGroup(groupId, role, normalizedName, normalizedCode, permissionBits);
    syncChatViewModelState();
}

void AppController::setMediaUploadInProgress(bool inProgress)
{
    if (_media_upload_state.inProgress == inProgress)
    {
        return;
    }
    _media_upload_state.inProgress = inProgress;
    syncChatViewModelState();
}

void AppController::setMediaUploadProgressText(const QString& text)
{
    if (_media_upload_state.progressText == text)
    {
        return;
    }
    _media_upload_state.progressText = text;
    syncChatViewModelState();
}

void AppController::setCurrentDraftText(const QString& text)
{
    if (!_features.chatFeatureController.setCurrentDraftText(text))
    {
        return;
    }
    syncChatViewModelState();
}

void AppController::setCurrentDialogPinned(bool pinned)
{
    if (!_features.chatFeatureController.setCurrentDialogPinned(pinned))
    {
        return;
    }
    syncChatViewModelState();
}

void AppController::setCurrentDialogMuted(bool muted)
{
    if (!_features.chatFeatureController.setCurrentDialogMuted(muted))
    {
        return;
    }
    syncChatViewModelState();
}

void AppController::setPendingReplyContext(const QString& msgId, const QString& senderName, const QString& previewText)
{
    if (!_features.chatFeatureController.setPendingReplyContext(msgId, senderName, previewText))
    {
        return;
    }
    syncChatViewModelState();
}
