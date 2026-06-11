#include "AppController.h"

#include "AppCoordinators.h"

#include <utility>

void AppController::bindChatFeatureMediaPorts()
{
    ChatPrivateCommandPort privateCommandPort;
    privateCommandPort.sendCurrentComposerPayload = [this](const QString& text)
    {
        _media_coordinator->sendCurrentComposerPayload(text);
    };
    privateCommandPort.sendImageMessage = [this]()
    {
        _media_coordinator->sendImageMessage();
    };
    privateCommandPort.sendFileMessage = [this]()
    {
        _media_coordinator->sendFileMessage();
    };

    ChatPendingAttachmentPort pendingAttachmentPort;
    pendingAttachmentPort.removePendingAttachment = [this](const QString& attachmentId)
    {
        _media_coordinator->removePendingAttachment(attachmentId);
    };
    pendingAttachmentPort.clearPendingAttachments = [this]()
    {
        _media_coordinator->clearPendingAttachments();
    };

    _features.chatFeatureController.setPrivateCommandPort(std::move(privateCommandPort));
    _features.chatFeatureController.setPendingAttachmentPort(std::move(pendingAttachmentPort));
}
