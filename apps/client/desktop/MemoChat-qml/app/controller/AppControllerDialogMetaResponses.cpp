#include "AppController.h"
#include "usermgr.h"

#include <QJsonObject>

void AppController::handleDialogMetaRsp(ReqId reqId, const QJsonObject& payload)
{
    ChatDialogMetaResponseRequest request;
    request.reqId = static_cast<int>(reqId);
    request.payload = payload;
    request.currentDialogUid = currentDialogUid();
    request.ownerUid = _gateway.userMgr()->GetUid();
    request.groupDialogUidMap = _features.chatFeatureController.groupDialogUidMapPtr();

    ChatDialogMetaResponseDependencies dependencies;
    dependencies.syncCurrentDraftText = [this](const QString& text)
    {
        setCurrentDraftText(text);
    };
    dependencies.syncCurrentDialogMuted = [this](bool muted)
    {
        setCurrentDialogMuted(muted);
    };
    dependencies.syncCurrentDialogPinned = [this](bool pinned)
    {
        setCurrentDialogPinned(pinned);
    };
    dependencies.savePersistentDialogStore = [this](int ownerUid)
    {
        saveDraftStore(ownerUid);
    };
    dependencies.refreshDialogModel = [this]()
    {
        refreshDialogModel();
    };
    _features.chatFeatureController.handleDialogMetaResponse(request, dependencies);
}
