#include "AppController.h"

void AppController::bindChatFeatureController()
{
    bindChatFeatureProjectionPort();
    bindChatFeatureDialogPorts();
    bindChatFeatureHistoryPorts();
    bindChatFeatureGroupPorts();
    bindChatFeatureMediaPorts();
    bindChatFeatureSendPorts();
    bindChatFeatureReadMutationPorts();
    _features.chatFeatureController.bindRequests();
}
