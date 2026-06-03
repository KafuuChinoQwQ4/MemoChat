#ifndef CHATUISTATE_H
#define CHATUISTATE_H

#include <QString>
#include <QVariantList>

struct ChatUiState
{
    int chatTab = 0;
    int currentDialogUid = 0;
    QString currentChatPeerName;
    QString currentChatPeerIcon = QStringLiteral("qrc:/res/head_1.png");
    bool hasCurrentChat = false;
    bool hasCurrentGroup = false;
    int currentGroupRole = 0;
    QString currentDraftText;
    QVariantList currentPendingAttachments;
    bool currentDialogPinned = false;
    bool currentDialogMuted = false;
    bool hasPendingReply = false;
    QString replyTargetName;
    QString replyPreviewText;
    bool dialogsReady = false;
    bool chatLoadingMore = false;
    bool canLoadMoreChats = false;
    bool privateHistoryLoading = false;
    bool canLoadMorePrivateHistory = false;
    bool mediaUploadInProgress = false;
    QString mediaUploadProgressText;
};

#endif // CHATUISTATE_H
