#include "AppController.h"

#include "AppCoordinators.h"

void AppController::sendTextMessage(const QString& text)
{
    _media_coordinator->sendTextMessage(text);
}

void AppController::sendCurrentComposerPayload(const QString& text)
{
    _media_coordinator->sendCurrentComposerPayload(text);
}

void AppController::sendImageMessage()
{
    _media_coordinator->sendImageMessage();
}

void AppController::sendFileMessage()
{
    _media_coordinator->sendFileMessage();
}

void AppController::removePendingAttachment(const QString& attachmentId)
{
    _media_coordinator->removePendingAttachment(attachmentId);
}

void AppController::clearPendingAttachments()
{
    _media_coordinator->clearPendingAttachments();
}
