#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <QtGlobal>

struct AppDialogRuntimeState
{
    QString currentDraftText;
    QVariantList currentPendingAttachments;
    QHash<int, QVariantList> pendingAttachmentMap;
    bool currentPinned = false;
    bool currentMuted = false;
    QString replyToMsgId;
    QString replyTargetName;
    QString replyPreviewText;
    QHash<int, QString> draftMap;
    QSet<int> localPinnedSet;
    QHash<int, int> serverPinnedMap;
    QHash<int, int> serverMuteMap;
    QHash<int, int> mentionMap;
    int lastEmittedUid = 0;
};

struct AppPrivateHistoryState
{
    qint64 beforeTs = 0;
    QString beforeMsgId;
    qint64 pendingBeforeTs = 0;
    QString pendingBeforeMsgId;
    int pendingPeerUid = 0;
};
