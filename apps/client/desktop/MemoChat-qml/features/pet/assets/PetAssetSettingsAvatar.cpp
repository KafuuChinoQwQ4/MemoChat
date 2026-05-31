#include "PetAssetSettings.h"
#include "PetAvatarResolver.h"

QString PetAssetSettings::live2dAvatarUrl() const
{
    return resolveLive2DAvatarUrl(_model_json, _model_root);
}

QString PetAssetSettings::resolveLive2DAvatarUrl(const QString& modelJson, const QString& modelRoot) const
{
    return memochat::pet_asset_settings::resolveLive2DAvatarCacheUrl(modelJson, modelRoot);
}
