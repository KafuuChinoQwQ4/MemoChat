#pragma once

#include "UserMessageData.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVector>
#include <QtGlobal>

#include <memory>

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

struct PendingIncomingMessage
{
    enum class Kind
    {
        Private,
        Group
    };

    Kind kind = Kind::Private;
    std::shared_ptr<TextChatMsg> privateMsg;
    std::shared_ptr<GroupChatMsg> groupMsg;
    QString key;
    qint64 createdAt = 0;
    qint64 sequence = 0;
};

struct AppIncomingMessageBufferState
{
    QVector<PendingIncomingMessage> messages;
    QSet<QString> keys;
    qint64 nextSequence = 0;
    static constexpr qsizetype maxMessages = 1000;

    void clear()
    {
        messages.clear();
        keys.clear();
        nextSequence = 0;
    }
};
