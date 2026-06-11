#include "GroupConversationServiceInternal.h"

using namespace memochat::chat::group_conversation;

GroupIncomingResult GroupConversationService::handleIncomingMessage(const GroupIncomingRequest& request,
                                                                    const GroupConversationDependencies& dependencies)
{
    GroupIncomingResult result;
    if (!hasUsableState(dependencies) || request.context.selfUid <= 0 || !request.message || !request.message->_msg)
    {
        return result;
    }

    result.groupId = request.message->_group_id;
    result.dialogUid = resolveGroupDialogUid(dependencies, result.groupId);
    bool ensuredGroup = false;
    ensureGroup(dependencies, result.groupId, request.message, result.dialogUid, &ensuredGroup);
    result.ensuredGroup = ensuredGroup;

    auto textMsg = MessagePayloadService::buildGroupIncomingMessage(*request.message);
    if (!textMsg)
    {
        return result;
    }

    if (dependencies.upsertGroupMessage)
    {
        dependencies.upsertGroupMessage(result.groupId, textMsg);
    }
    result.cacheUpdated = cacheGroupMessages(dependencies, request.context.selfUid, result.groupId, {textMsg});

    const QString preview = MessageContentCodec::toPreviewText(request.message->_msg->_msg_content);
    const bool fromSelf = request.message->_from_uid == request.context.selfUid;
    result.mentionedMe = mentionsSelf(*request.message, request.context.selfUid);
    updateGroupPreview(dependencies,
                       result.dialogUid,
                       preview,
                       result.mentionedMe
                       ? QStringLiteral("[有人@你] %1").arg(preview) : preview,
                                        request.message->_msg->_created_at,
                                        &result.previewUpdated);

    const bool viewingCurrentGroup =
        request.context.currentGroupId == result.groupId && request.context.currentPrivatePeerUid <= 0;
    if (viewingCurrentGroup)
    {
        clearUnread(dependencies, result.dialogUid, &result.unreadCleared);
    }
    else if (!fromSelf)
    {
        incrementUnread(dependencies, result.dialogUid, result.mentionedMe, &result.unreadIncremented);
    }

    if (viewingCurrentGroup && dependencies.messageModel)
    {
        dependencies.messageModel->appendMessage(textMsg, request.context.selfUid);
        result.modelAppended = true;
        result.requestedReadAckTs = textMsg->_created_at;
    }

    result.success = true;
    return result;
}
