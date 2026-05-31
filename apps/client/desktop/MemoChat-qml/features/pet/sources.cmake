list(APPEND MEMOCHAT_QML_FEATURE_SOURCES
    features/pet/assets/PetAssetSettings.cpp
    features/pet/assets/PetAssetSettingsAvatar.cpp
    features/pet/assets/PetAssetSettingsPersistence.cpp
    features/pet/assets/PetAssetSettingsState.cpp
    features/pet/assets/PetAssetSettingsVoiceTraining.cpp
    features/pet/assets/PetAvatarPathUtils.cpp
    features/pet/assets/PetAvatarResolver.cpp
    features/pet/controller/PetController.cpp
    features/pet/controller/PetControllerNetwork.cpp
    features/pet/controller/PetControllerSession.cpp
    features/pet/controller/PetControllerState.cpp
    features/pet/speech/PetControllerVoiceRuntime.cpp
    features/pet/speech/PetControllerVoiceTraining.cpp
    features/pet/speech/PetSpeechSynthesizer.cpp
    features/pet/vision/PetVisionFrameEncoder.cpp
    features/pet/vision/PetControllerVision.cpp
    features/pet/platform/PetControllerWindowsBridge.cpp
    features/pet/model/PetModel.cpp
)

list(APPEND MEMOCHAT_QML_FEATURE_HEADERS
    features/pet/assets/PetAssetSettings.h
    features/pet/assets/PetAssetSettingsPrivate.h
    features/pet/assets/PetAvatarPathUtils.h
    features/pet/assets/PetAvatarResolver.h
    features/pet/controller/PetController.h
    features/pet/controller/PetNetworkRequestUtils.h
    features/pet/controller/PetControllerPrivate.h
    features/pet/speech/PetSpeechSynthesizer.h
    features/pet/vision/PetVisionFrameEncoder.h
    features/pet/vision/PetVisionFrameUtils.h
    features/pet/platform/PetWindowsBridgeUtils.h
    features/pet/model/PetModel.h
)
