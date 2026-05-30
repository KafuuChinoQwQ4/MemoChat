#include "MainQmlBootstrap.h"

#include "AppController.h"
#include "CallSessionModel.h"
#include "Live2DAsset.h"
#include "Live2DRenderItem.h"
#include "PetAssetSettings.h"
#include "PetController.h"
#include "PetModel.h"
#include "PetSpeechSynthesizer.h"

#include <QtQml>

void registerMemoChatQmlTypes()
{
    qmlRegisterUncreatableType<AppController>("MemoChat", 1, 0, "AppController", "Enum only");
    qmlRegisterUncreatableType<CallSessionModel>("MemoChat", 1, 0, "CallSessionModel", "Exposed via AppController");
    qmlRegisterUncreatableType<PetController>("MemoChat", 1, 0, "PetController", "Exposed via AppController");
    qmlRegisterUncreatableType<PetModel>("MemoChat", 1, 0, "PetModel", "Exposed via PetController");
    qmlRegisterType<PetSpeechSynthesizer>("MemoChat", 1, 0, "PetSpeechSynthesizer");
    qmlRegisterType<PetAssetSettings>("MemoChat", 1, 0, "PetAssetSettings");
    qmlRegisterType<Live2DAsset>("MemoChat", 1, 0, "Live2DAsset");
    qmlRegisterType<Live2DRenderItem>("MemoChat", 1, 0, "Live2DRenderItem");
}
