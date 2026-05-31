import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
PET_MODEL_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetModel.h"
PET_MODEL_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetModel.cpp"
PET_CONTROLLER_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetController.h"
PET_SCENE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetScene.qml"
PET_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetWindow.qml"
PET_WINDOW_RUNTIME_JS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetWindowRuntime.js"
PET_CHAT_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetChatWindow.qml"
PET_CHAT_RUNTIME_JS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetChatRuntime.js"
PET_CHAT_MESSAGE_LIST_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetChatMessageList.qml"
PET_CHAT_COMPOSER_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetChatComposer.qml"
CHAT_COMPOSER_BAR_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/conversation/ChatComposerBar.qml"
CHAT_MESSAGE_DELEGATE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/conversation/ChatMessageDelegate.qml"
CHAT_MESSAGE_AVATAR_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/conversation/MessageAvatar.qml"
PET_CONTROL_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlWindow.qml"
PET_CONTROL_RUNTIME_JS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlRuntime.js"
PET_CONTROL_LIVE2D_ACTION_PANEL_QML = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlLive2DActionPanel.qml"
)
PET_CONTROL_API_PROVIDER_PANEL_QML = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlApiProviderPanel.qml"
)
PET_CONTROL_HEADER_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlHeader.qml"
PET_VISION_PRIVACY_CARD_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetVisionPrivacyCard.qml"
CHARACTER_PANE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/Live2DCharacterPane.qml"
LIVE2D_CHARACTER_RUNTIME_JS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/Live2DCharacterRuntime.js"
LIVE2D_CHARACTER_PREVIEW_PANEL_QML = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/Live2DCharacterPreviewPanel.qml"
)
LIVE2D_RESOURCE_VOICE_PANEL_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/Live2DResourceVoicePanel.qml"
LIVE2D_BEHAVIOR_MEMORY_PANEL_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/Live2DBehaviorMemoryPanel.qml"
LIVE2D_CHARACTER_BEHAVIOR_COLUMN_QML = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/Live2DCharacterBehaviorColumn.qml"
)
CHAT_SHELL_PAGE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml"
CHAT_NORMAL_FACE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/chat/ChatNormalFace.qml"
SHARED_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/Main.qml"
LINUX_MAIN_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/linux/Main.qml"
PET_CONTROLLER_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetController.h"
PET_CONTROLLER_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetController.cpp"
PET_CONTROLLER_PRIVATE_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerPrivate.h"
PET_CONTROLLER_NETWORK_UTILS_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetNetworkRequestUtils.h"
PET_CONTROLLER_VISION_ENCODER_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetVisionFrameEncoder.h"
PET_CONTROLLER_VISION_ENCODER_CPP = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetVisionFrameEncoder.cpp"
)
PET_CONTROLLER_VISION_UTILS_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetVisionFrameUtils.h"
PET_CONTROLLER_WINDOWS_UTILS_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetWindowsBridgeUtils.h"
PET_CONTROLLER_NETWORK_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerNetwork.cpp"
PET_CONTROLLER_VISION_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerVision.cpp"
PET_CONTROLLER_WINDOWS_BRIDGE_CPP = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerWindowsBridge.cpp"
)
CLIENT_DIR = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
CLIENT_CMAKE = CLIENT_DIR / "CMakeLists.txt"
CLIENT_CMAKE_FRAGMENTS = (
    CLIENT_DIR / "cmake/AppSources.cmake",
    CLIENT_DIR / "cmake/FeatureSources.cmake",
    CLIENT_DIR / "cmake/SharedSources.cmake",
    CLIENT_DIR / "cmake/QmlResources.cmake",
)
QML_QRC = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml.qrc"
PET_CAMERA_CAPTURE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetCameraCapture.qml"
MAIN_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/main.cpp"
MAIN_PLATFORM_BOOTSTRAP_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/app/MainPlatformBootstrap.cpp"
PET_CONTROLLER_SESSION_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerSession.cpp"
PET_CONTROLLER_VOICE_TRAINING_CPP = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerVoiceTraining.cpp"
)


def read_texts(*paths):
    return "\n".join(path.read_text(encoding="utf-8") for path in paths)


def client_cmake_text() -> str:
    return read_texts(CLIENT_CMAKE, *(fragment for fragment in CLIENT_CMAKE_FRAGMENTS if fragment.exists()))


def compact_ws(source: str) -> str:
    return " ".join(source.split())


class PetQmlContractTests(unittest.TestCase):
    def test_pet_model_exposes_v1_event_metadata(self):
        header = PET_MODEL_H.read_text(encoding="utf-8")
        controller = PET_CONTROLLER_H.read_text(encoding="utf-8")

        for prop in (
            "schemaVersion",
            "eventId",
            "turnId",
            "phase",
            "speechTranslation",
            "speechDisplayText",
            "speechLanguage",
            "speechFinal",
            "audioUrl",
            "audioState",
        ):
            self.assertIn(prop, header)
            self.assertIn(prop, controller)

    def test_pet_model_parses_v1_nested_fields_with_legacy_fallbacks(self):
        source = PET_MODEL_CPP.read_text(encoding="utf-8")

        for token in (
            'QStringLiteral("animation")',
            'QStringLiteral("audio")',
            'QStringLiteral("url")',
            'QStringLiteral("state")',
            'QStringLiteral("text")',
            'QStringLiteral("delta")',
            'QStringLiteral("display")',
            'QStringLiteral("translation")',
            'QStringLiteral("final")',
            'QStringLiteral("speech")',
            'QStringLiteral("lip_sync")',
        ):
            self.assertIn(token, source)

        self.assertIn("_speech_turn_id", source)
        self.assertIn("displayTextForSpeech", source)
        self.assertIn("jsonBool", source)
        self.assertIn("_audio_url", source)
        self.assertIn("_audio_state", source)

    def test_pet_scene_keeps_null_controller_and_phase_status_guards(self):
        scene = PET_SCENE_QML.read_text(encoding="utf-8")

        self.assertIn(': (root.petController ? root.petController.expression : "neutral")', scene)
        self.assertIn(': (root.petController ? root.petController.motion : "idle")', scene)
        self.assertIn("PetAudioPlayer.qml", scene)
        self.assertIn("root.petController.audioUrl", scene)
        self.assertIn("root.petController.audioState", scene)
        self.assertIn("voiceReplyEnabled", scene)
        self.assertIn("active: root.voiceReplyEnabled && root.petController", scene)
        self.assertIn("function isObservationVisualEvent", scene)
        self.assertIn("function voiceReplyIsActive", scene)
        self.assertIn("petAudioLoader.item.playbackActive", scene)
        self.assertIn('item.sourceUrl = root.petController ? root.petController.audioUrl : ""', scene)
        self.assertIn('item.playbackState = root.petController ? root.petController.audioState : "idle"', scene)
        self.assertIn("speechBubbleText", scene)
        self.assertIn("speechBubbleTranslation", scene)
        self.assertIn("speechBubbleJapanese", scene)
        self.assertIn("speechBubbleVisible", scene)
        self.assertIn("id: speechBubble", scene)
        self.assertIn("root.petController.speechDisplayText.trim()", scene)
        self.assertIn("root.petController.speechTranslation.trim()", scene)
        self.assertIn('root.petController.speechLanguage.toLowerCase().indexOf("ja") === 0', scene)
        self.assertIn("width: Math.min(bubbleMaxWidth, 180)", scene)
        self.assertIn("height: 72", scene)
        self.assertIn("readonly property int speechBubbleSafeHeight: 84", scene)
        self.assertIn("anchors.topMargin: root.speechBubbleSafeHeight", scene)
        self.assertIn("visible: root.speechBubbleTranslation.length > 0", scene)
        self.assertNotIn("speechBubbleReservedHeight", scene)
        self.assertNotIn("anchors.topMargin: root.speechBubbleReservedHeight", scene)
        self.assertNotIn("readonly property real uiScale", scene)
        self.assertNotIn("speechFinal", scene)
        self.assertNotIn("speechPlaybackText", scene)
        self.assertNotIn("textToSpeechFallbackEnabled", scene)
        self.assertNotIn("audioPlaybackActive", scene)
        self.assertNotIn("capturePaused", scene)
        self.assertNotIn("anchors.topMargin: speechBubble.visible", scene)

    def test_pet_audio_player_resource_declares_qt_multimedia(self):
        qrc = QML_QRC.read_text(encoding="utf-8")
        cmake = client_cmake_text()
        audio_player = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetAudioPlayer.qml").read_text(
            encoding="utf-8"
        )

        self.assertIn("qml/pet/PetAudioPlayer.qml", qrc)
        self.assertIn("find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Multimedia)", cmake)
        self.assertIn("Qt${QT_VERSION_MAJOR}::Multimedia", cmake)
        self.assertNotIn("TextToSpeech", cmake)
        self.assertIn("import QtMultimedia", audio_player)
        self.assertIn("MediaPlayer", audio_player)
        self.assertIn("function hasPlayableAudio", audio_player)
        self.assertIn("return sourceUrl.length > 0", audio_player)
        self.assertIn("property string lastStartedAudioKey", audio_player)
        self.assertIn("property bool playbackActive", audio_player)
        self.assertIn("function refreshPlaybackActivity", audio_player)
        self.assertIn("function audioPlaybackKey", audio_player)
        self.assertIn('if (name === "event" || name === "turn")', audio_player)
        self.assertIn("function playableAudioKey", audio_player)
        self.assertIn("function playPlayableAudio", audio_player)
        self.assertIn("root.lastStartedAudioKey === key", audio_player)
        self.assertIn("root.lastStartedAudioKey = key", audio_player)
        self.assertIn("MediaPlayer.StoppedState", audio_player)
        self.assertIn("property string playbackState", audio_player)
        self.assertIn("function canPlayForState", audio_player)
        self.assertIn('playbackState === "ready"', audio_player)
        self.assertIn('playbackState === "playing"', audio_player)
        self.assertIn('playbackState === "interrupted"', audio_player)
        self.assertIn("petAudioPlayer.stop()", audio_player)
        self.assertIn("petAudioPlayer.play()", audio_player)
        self.assertNotIn("speechText", audio_player)
        self.assertNotIn("speechLanguage", audio_player)
        self.assertNotIn("speechFinal", audio_player)
        self.assertNotIn("textToSpeechFallbackEnabled", audio_player)
        self.assertNotIn("PetSpeechSynthesizer", audio_player)
        self.assertNotIn("QtTextToSpeech", audio_player)
        self.assertNotIn("nativeSpeech", audio_player)
        self.assertNotIn("deterministic-voice-", audio_player)

    def test_pet_controller_audio_url_is_absolute_and_cache_busted_per_event(self):
        source = read_texts(
            PET_CONTROLLER_CPP, REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerState.cpp"
        )

        self.assertIn("gate_media_url_prefix", source)
        self.assertIn('return QUrl(gate_url_prefix.trimmed() + QStringLiteral("/ai/pet") + path);', source)
        self.assertIn("if (port == 8096)", source)
        self.assertIn('base.setPath(QStringLiteral("/pet") + path)', source)
        self.assertIn('base.setPath(QStringLiteral("/ai/pet") + path)', source)
        self.assertIn("QUrlQuery query(url)", source)
        self.assertIn("QUrlQuery query(base)", source)
        self.assertIn('query.addQueryItem(QStringLiteral("turn"), _model.turnId())', source)
        self.assertIn('query.addQueryItem(QStringLiteral("event"), _model.eventId())', source)
        self.assertIn("url.setQuery(query)", source)
        self.assertIn("return url.toString()", source)

    def test_pet_controller_configures_direct_https_requests_like_shared_http(self):
        source = read_texts(PET_CONTROLLER_PRIVATE_H, PET_CONTROLLER_NETWORK_UTILS_H, PET_CONTROLLER_NETWORK_CPP)

        self.assertRegex(source, r"configurePetRequest\s*\(\s*QNetworkRequest\s*&\s*request\s*\)")
        self.assertIn("sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);", source)
        self.assertIn("request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);", source)
        self.assertIn("configurePetRequest(request);", source)

    def test_pet_controller_large_concerns_stay_split_from_main_controller_file(self):
        cmake = client_cmake_text()
        source = PET_CONTROLLER_CPP.read_text(encoding="utf-8")

        for token in (
            "features/pet/PetControllerNetwork.cpp",
            "features/pet/PetControllerVision.cpp",
            "features/pet/PetControllerWindowsBridge.cpp",
            "features/pet/PetControllerPrivate.h",
            "features/pet/PetNetworkRequestUtils.h",
            "features/pet/PetVisionFrameEncoder.cpp",
            "features/pet/PetVisionFrameEncoder.h",
            "features/pet/PetVisionFrameUtils.h",
            "features/pet/PetWindowsBridgeUtils.h",
        ):
            self.assertIn(token, cmake)

        for token in (
            "void PetController::postJson",
            "void PetController::startStream",
            "bool PetController::captureVisionVideoFrame",
            "bool PetController::captureVisionWindowsCameraFrame",
            "void PetController::openWindowsImeBridge",
        ):
            self.assertNotIn(token, source)

    def test_pet_vision_frame_encoding_is_split_from_controller_state(self):
        cmake = client_cmake_text()
        vision = PET_CONTROLLER_VISION_CPP.read_text(encoding="utf-8")
        encoder = read_texts(PET_CONTROLLER_VISION_ENCODER_H, PET_CONTROLLER_VISION_ENCODER_CPP)

        self.assertIn("features/pet/PetVisionFrameEncoder.cpp", cmake)
        self.assertIn("features/pet/PetVisionFrameEncoder.h", cmake)
        self.assertIn("encodeVisionVideoFrameAsJpeg(frame, 82)", vision)
        self.assertIn("encodeVisionVideoFrameAsJpeg(frame, 72)", vision)
        self.assertIn("readVisionFrameFile(localPath)", vision)
        self.assertIn("struct EncodedVisionFrame", encoder)
        self.assertIn("EncodedVisionFrame encodeVisionImageAsJpeg", encoder)
        self.assertIn("QString visionFrameFileMime", encoder)

    def test_pet_controller_keeps_post_events_with_sse_streaming(self):
        source = read_texts(PET_CONTROLLER_CPP, PET_CONTROLLER_SESSION_CPP, PET_CONTROLLER_NETWORK_CPP)
        compact = compact_ws(source)

        self.assertIn("if (!_streaming) { startStream(); }", compact)
        self.assertNotIn("if (!_streaming) { const QJsonArray events", compact)
        self.assertIn('const QJsonArray events = root.value(QStringLiteral("events")).toArray();', source)

    def test_pet_controller_uses_rolling_vision_segment_buffer(self):
        source = read_texts(
            PET_CONTROLLER_H,
            PET_CONTROLLER_PRIVATE_H,
            PET_CONTROLLER_VISION_UTILS_H,
            PET_CONTROLLER_CPP,
            PET_CONTROLLER_VISION_CPP,
        )

        for token in (
            "kVisionSegmentMaxFrames = 14",
            "kVisionSegmentWindowMs = 20000",
            "kVisionSegmentMinUploadMs = 15000",
            "kVisionDuplicateFrameCooldownMs = 30000",
            "kVisionFrameSignatureSize = 16",
            "kVisionFrameSignatureDuplicateDistance = 32",
            "visionFrameSignature",
            "visionFrameSignatureDistance",
            "shouldSkipVisionFrame",
            "frame_signature",
            "captured_at_ms",
            "normalizeVisionSegmentFrames",
            "视觉环形缓冲采样中",
            "视觉画面未变化，已跳过",
            "_vision_segment_last_posted_at_ms",
            "_last_vision_frame_signature",
            "_last_vision_frame_accepted_at_ms",
            "_vision_request_in_flight",
            "setVisionRequestInFlight",
            "_vision_segment_frames = QJsonArray();",
        ):
            self.assertIn(token, source)
        self.assertNotIn("kVisionSegmentMaxDurationMs", source)

    def test_pet_camera_capture_uses_latest_frame_mailbox_and_analysis_tick(self):
        qml = PET_CAMERA_CAPTURE_QML.read_text(encoding="utf-8")

        for token in (
            "analysisIntervalMs: 1000",
            "latestFrameHoldMs: 1800",
            "latestVideoFrame",
            "latestVideoFrameSequence",
            "latestSubmittedFrameSequence",
            "latestFrameCapturePending",
            "root.latestFrameCapturePending = true",
            "root.latestFrameCapturePending = false",
            "visionBusy()",
            "visionRequestInFlight",
            "摄像头最新帧分析中",
            "摄像头最新帧已提交分析",
            "摄像头缓存帧等待回调",
            "摄像头视频流未就绪，使用单帧兜底",
            "摄像头画面已过期，等待新帧",
            "cameraStatusText()",
        ):
            self.assertIn(token, qml)

    def test_pet_speech_state_clears_before_new_manual_input(self):
        model = PET_MODEL_CPP.read_text(encoding="utf-8")
        controller = read_texts(PET_CONTROLLER_CPP, PET_CONTROLLER_SESSION_CPP)
        compact_model = compact_ws(model)

        self.assertIn("&& !_speech_final && _audio_url.isEmpty()", compact_model)
        self.assertIn("_speech_final = false;", model)
        self.assertIn("_model.clearSpeech();", controller)

    def test_pet_model_only_updates_speech_language_when_text_changes(self):
        model = PET_MODEL_CPP.read_text(encoding="utf-8")

        self.assertIn("const bool has_text_update", model)
        self.assertIn(
            "has_text_update && !preserve_voice_for_observation && updateString(_speech_language, text_language)", model
        )

    def test_pet_model_preserves_active_voice_for_observation_events(self):
        model = PET_MODEL_CPP.read_text(encoding="utf-8")

        self.assertIn("preserve_voice_for_observation", model)
        self.assertIn("isObservationControlEvent", model)
        self.assertIn("isAudioStateStillPresenting", model)
        self.assertIn('actionName == QStringLiteral("observe")', model)
        self.assertIn('actionName == QStringLiteral("visual_react")', model)
        self.assertIn("!preserve_voice_for_observation", model)
        self.assertIn(
            "reset_speech_for_waiting_phase && !_audio_url.isEmpty() && !preserve_voice_for_observation", model
        )

    def test_pet_chat_window_exposes_wsl_windows_ime_bridge(self):
        header = PET_CONTROLLER_H.read_text(encoding="utf-8")
        source = read_texts(
            PET_CONTROLLER_CPP,
            PET_CONTROLLER_SESSION_CPP,
            REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/pet/PetControllerVoiceRuntime.cpp",
            PET_CONTROLLER_PRIVATE_H,
            PET_CONTROLLER_WINDOWS_UTILS_H,
            PET_CONTROLLER_WINDOWS_BRIDGE_CPP,
        )
        chat = read_texts(PET_CHAT_WINDOW_QML, PET_CHAT_RUNTIME_JS, PET_CHAT_MESSAGE_LIST_QML, PET_CHAT_COMPOSER_QML)
        composer = CHAT_COMPOSER_BAR_QML.read_text(encoding="utf-8")

        for token in (
            "windowsImeBridgeAvailable",
            "windowsImeBridgeBusy",
            "windowsImeTextCommitted",
            "openWindowsImeBridge",
            "selectedModelType",
            "selectedModelName",
            "replyLanguage",
            "speechRules",
            "setModelSelection",
            "setReplyLanguage",
            "setSpeechRules",
            "setVoiceRuntimeSettings",
        ):
            self.assertIn(token, header)

        for token in (
            "setModelSelection",
            "setReplyLanguage",
            "setSpeechRules",
            "setVoiceRuntimeSettings",
            "voiceRuntimeMetadata",
            "appendVoiceRuntimeMetadata",
            'payload[QStringLiteral("model_type")]',
            'payload[QStringLiteral("model_name")]',
            'metadata[QStringLiteral("reply_language")]',
            'metadata[QStringLiteral("speech_rules")]',
            'metadata[QStringLiteral("voice_provider")]',
            'metadata[QStringLiteral("ref_audio_path")]',
            "appendVoiceRuntimeMetadata(metadata)",
        ):
            self.assertIn(token, source)

        for token in (
            "/proc/sys/kernel/osrelease",
            "powershell.exe",
            "-EncodedCommand",
            "System.Windows.Forms.TextBox",
            "ImeMode",
            "Microsoft YaHei UI",
        ):
            self.assertIn(token, source)

        for token in (
            "ListModel",
            "ListView",
            "ChatMessageDelegate",
            "function chatWindowFlags()",
            "flags: chatWindowFlags()",
            "TextArea",
            "CompactIconButton",
            "messageInput",
            "sendButton",
            "messageSenderName",
            "showOutgoingSenderName: true",
            "voiceChatRequested",
            "videoChatRequested",
            "appendOrUpdateAssistantMessage",
            "syncModelSelection",
            "syncReplyLanguage",
            "syncSpeechRules",
            "syncVoiceRuntimeSettings",
            "root.petController.setVoiceRuntimeSettings",
            "referenceAudioPath",
            "voiceTrainingArtifactPath",
            "speechRulesText",
            "pendingAssistantIndex",
            "pendingAssistantTurnId",
            "sendPendingText",
            "root.petController.startSession()",
            "root.petController.sendText(trimmed)",
            "function assistantEventSnapshot",
            "function assistantControllerSnapshot",
            "function assistantFinalStatus",
            "function assistantProgressStatus",
            "PetChatRuntime.assistantEventSnapshot",
            "PetChatRuntime.assistantControllerSnapshot(root.petController)",
            "assistantEvent.audioReadyPayload",
            "assistantEvent.resolvedEventKey",
            "root.updateCompletedAssistantAudio(assistantEvent.resolvedEventKey",
            "function handleControllerError",
            'messageModel.setProperty(root.pendingAssistantIndex, "messageState", "failed")',
            'root.chatStatusText = "发送失败，可重试"',
            "root.petController.setModelSelection",
            "root.petController.setReplyLanguage",
            "root.petController.setSpeechRules",
            "onWindowsImeBridgeChanged",
            "messageInput.forceActiveFocus()",
        ):
            self.assertIn(token, chat)

        self.assertIn("imeBridgeController", composer)
        self.assertIn("focusMessageInput", composer)
        self.assertIn("preeditText", composer)
        self.assertIn("pendingDraftSync", composer)
        self.assertIn("syncDraftTextFromBinding", composer)
        self.assertIn("scheduleInputMethodUpdate", composer)
        self.assertIn("Qt.inputMethod.update(Qt.ImQueryAll)", composer)
        self.assertIn("onCursorRectangleChanged", composer)
        self.assertNotIn("imeBridgeActive", composer)
        self.assertNotIn("preferWindowsImeBridge", composer)
        self.assertNotIn("openImeBridge", composer)
        self.assertNotIn("pinyinCandidateMap", composer)
        self.assertNotIn("currentPinyinToken", composer)
        self.assertNotIn("pinyinCandidates", composer)
        self.assertNotIn("commitPinyinCandidate", composer)
        self.assertNotIn("insertCommittedText", composer)
        self.assertNotIn("replaceDraftText", composer)
        self.assertNotIn("onWindowsImeTextCommitted", composer)
        self.assertNotIn("Qt.inputMethod.show()", composer)
        self.assertNotIn("Keys.onPressed", composer)
        self.assertNotIn("pinyinCandidateBar", composer)
        self.assertNotIn('text: "中"', composer)

    def test_pet_audio_player_skips_text_to_speech_fallback(self):
        audio_player = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetAudioPlayer.qml").read_text(
            encoding="utf-8"
        )

        self.assertIn("MediaPlayer", audio_player)
        self.assertIn("function hasPlayableAudio", audio_player)
        self.assertIn("return sourceUrl.length > 0", audio_player)
        self.assertIn("function playPlayableAudio", audio_player)
        self.assertNotIn("QtTextToSpeech", audio_player)
        self.assertNotIn("PetSpeechSynthesizer", audio_player)
        self.assertNotIn("nativeSpeech", audio_player)
        self.assertNotIn("speechText", audio_player)
        self.assertNotIn("speechLanguage", audio_player)
        self.assertNotIn("speechFinal", audio_player)
        self.assertNotIn("textToSpeechFallbackEnabled", audio_player)
        self.assertNotIn("deterministic-voice-", audio_player)

    def test_main_cpp_bootstraps_linux_input_method_env_for_chinese_entry(self):
        source = read_texts(MAIN_CPP, MAIN_PLATFORM_BOOTSTRAP_CPP)

        for token in (
            "configureLinuxInputMethod",
            "QT_IM_MODULE",
            "GTK_IM_MODULE",
            "XMODIFIERS",
            "fcitx5",
            "ibus-daemon",
            "startIbusDaemon",
            "stopIbusDaemons",
            "selectIbusLibpinyinEngine",
            "MEMOCHAT_RESTART_IBUS",
            "pgrep",
            "pkill",
            "startDetached",
            "libfcitx5platforminputcontextplugin.so",
            "libibusplatforminputcontextplugin.so",
            "libpinyin",
            "IBUS_ENABLE_SYNC_MODE",
            "--replace",
            "GDK_BACKEND",
            "WAYLAND_DISPLAY",
            "qunsetenv",
            'env.remove(QStringLiteral("WAYLAND_DISPLAY"))',
            "QT_QPA_PLATFORM",
            "qtvirtualkeyboard",
            "QT_VIRTUALKEYBOARD_DESKTOP_DISABLE",
        ):
            self.assertIn(token, source)

    def test_pet_camera_capture_upload_contract_uses_qt_multimedia(self):
        qrc = QML_QRC.read_text(encoding="utf-8")
        cmake = client_cmake_text()
        header = PET_CONTROLLER_H.read_text(encoding="utf-8")
        source = read_texts(
            PET_CONTROLLER_PRIVATE_H,
            PET_CONTROLLER_VISION_UTILS_H,
            PET_CONTROLLER_WINDOWS_UTILS_H,
            PET_CONTROLLER_VISION_CPP,
            PET_CONTROLLER_WINDOWS_BRIDGE_CPP,
        )
        scene = PET_SCENE_QML.read_text(encoding="utf-8")
        camera_capture = PET_CAMERA_CAPTURE_QML.read_text(encoding="utf-8")

        self.assertIn("qml/pet/PetCameraCapture.qml", qrc)
        self.assertIn("Qt${QT_VERSION_MAJOR}::Multimedia", cmake)
        for token in (
            "Q_INVOKABLE void captureVisionFrame",
            "Q_INVOKABLE bool captureVisionVideoFrame",
            "Q_INVOKABLE QString nextVisionCaptureFilePath",
            "Q_INVOKABLE void captureVisionFrameFile",
            "windowsCameraBridgeAvailable",
            "windowsCameraBridgeBusy",
            "Q_INVOKABLE bool captureVisionWindowsCameraFrame",
        ):
            self.assertIn(token, header)
        for token in (
            "/capture",
            "frame_base64",
            "frame_mime",
            "frame_width",
            "frame_height",
            "local_frame_upload",
            "qt_video_sink",
            "live_frame_upload",
            "windows_camera_bridge",
            "wsl_windows_camera_bridge",
            "Windows.Media.Capture.MediaCapture",
        ):
            self.assertIn(token, source)
        self.assertIn("PetCameraCapture.qml", scene)
        for token in (
            "import QtMultimedia",
            "MediaDevices",
            "Camera",
            "CaptureSession",
            "readonly property bool cameraAvailable",
            "readonly property bool qtCameraAvailable",
            "readonly property bool windowsCameraBridgeAvailable",
            "readonly property bool useWindowsCameraBridge",
            "mediaDevices.videoInputs.length",
            "onVideoInputsChanged",
            "未检测到摄像头",
            "Windows 摄像头桥已开启",
            "等待桌宠会话",
            "active: root.cameraEnabled && root.qtCameraAvailable",
            "videoOutput.videoSink",
            "onVideoFrameChanged",
            "property var liveVideoFrame: null",
            "captureVisionVideoFrame",
            "visionRequestInFlight",
            "visionBusy()",
            "latestVideoFrame",
            "latestVideoFrameSequence",
            "latestSubmittedFrameSequence",
            "analysisIntervalMs: 1000",
            "latestFrameHoldMs: 1800",
            "摄像头最新帧分析中",
            "摄像头最新帧已提交分析",
            "摄像头视频流未就绪，使用单帧兜底",
            "摄像头画面已过期，等待新帧",
            "captureVisionWindowsCameraFrame",
            "onWindowsCameraBridgeChanged",
            "ImageCapture",
            "VideoOutput",
            "captureToFile",
            "captureVisionFrameFile",
            "root.useWindowsCameraBridge ? 12000 : 1000",
        ):
            self.assertIn(token, camera_capture)
        self.assertNotIn("segmentCaptureEnabled ? 1500 : 4500", camera_capture)
        self.assertNotIn("capturePaused", camera_capture)

    def test_pet_vision_privacy_controls_have_guardrails_and_diagnostics(self):
        control = read_texts(PET_CONTROL_WINDOW_QML, PET_CONTROL_RUNTIME_JS)
        privacy_card = PET_VISION_PRIVACY_CARD_QML.read_text(encoding="utf-8")
        scene = PET_SCENE_QML.read_text(encoding="utf-8")
        window = PET_WINDOW_QML.read_text(encoding="utf-8")

        for token in (
            "function modelProviderAvailable",
            "function cloudVisionRuntimeEnabled",
            "function requestCloudVision",
            "function requestLocalOnlyMode",
            "root.cloudVisionToggled(false)",
            "enabled: !root.localOnlyMode && root.modelProviderAvailable()",
            "checked: root.cloudVisionRuntimeEnabled()",
            "root.cameraDiagnosticText()",
            "root.cloudVisionDiagnosticText()",
            "root.retentionDiagnosticText()",
            "原始帧不保留",
        ):
            self.assertIn(token, control)

        for token in (
            "视觉隐私",
            "cameraDiagnosticText",
            "cloudVisionDiagnosticText",
            "retentionDiagnosticText",
            "cloudVisionEnabled",
            "debugRetentionEnabled",
        ):
            self.assertIn(token, privacy_card)

        for token in (
            "function providerRuntimeAvailable",
            "function enforceVisionPrivacy",
            "function setCloudVisionEnabled",
            "function setLocalOnlyMode",
            "function applyAssetPrivacySettings",
            "root.petAssetSettings.cameraEnabled",
            "root.petAssetSettings.cloudVisionEnabled",
            "root.providerRuntimeAvailable()",
            "onModelChanged",
            "onModelsChanged",
            "onModelStateChanged",
            "root.setCloudVisionEnabled(value)",
            "root.setLocalOnlyMode(value)",
        ):
            self.assertIn(token, window)

        for token in (
            "云视觉 本地锁定",
            "云视觉 无提供方",
            "function visionDiagnosticText",
            "root.cameraCaptureStatus = item.statusText",
            "height: 118",
        ):
            self.assertIn(token, scene)

    def test_pet_window_is_transparent_model_first_and_resources_remain_registered(self):
        window = PET_WINDOW_QML.read_text(encoding="utf-8")
        window_runtime = PET_WINDOW_RUNTIME_JS.read_text(encoding="utf-8")
        scene = PET_SCENE_QML.read_text(encoding="utf-8")
        qrc = QML_QRC.read_text(encoding="utf-8")

        self.assertIn("Qt.WindowStaysOnTopHint", window)
        self.assertIn('Qt.platform.os === "linux"', window)
        self.assertIn("Qt.Window", window)
        self.assertIn("Qt.Tool", window)
        self.assertIn("root.startSystemMove()", window)
        self.assertIn("scaledWindowWidth", window)
        self.assertIn("scaledWindowHeight", window)
        self.assertIn("applyScale", window)
        self.assertIn("readonly property int speechBubbleSafeHeight: 84", window)
        self.assertIn('import "PetWindowRuntime.js" as PetWindowRuntime', window)
        self.assertIn("function scaledWindowHeight", window_runtime)
        self.assertIn("return speechBubbleSafeHeight + Math.round(baseWindowHeight * scaleFactor)", window_runtime)
        self.assertIn("minimumHeight: speechBubbleSafeHeight + Math.round(baseWindowHeight * 0.65)", window)
        self.assertIn("root.width = scaledWindowWidth(nextScale)", window)
        self.assertIn("root.height = scaledWindowHeight(nextScale)", window)
        self.assertNotIn("oldBottom", window)
        self.assertNotIn("oldCenterX", window)
        self.assertIn("positionControlWindow()", window)
        self.assertNotIn("root.width = Math.max(root.minimumWidth", window)
        self.assertNotIn("root.height = Math.max(root.minimumHeight", window)
        self.assertNotIn("anchors.fill: parent\n        radius: 18", window)
        self.assertIn("property var agentController: null", window)
        self.assertIn("property var petAssetSettings: null", window)
        self.assertIn("petAssetSettings: root.petAssetSettings", window)
        self.assertIn("function settingsLanguageCode", window)
        self.assertIn("function syncPetRuntimeSettings", window)
        self.assertIn("root.petController.setReplyLanguage(settingsLanguageCode())", window)
        self.assertIn("root.petController.setSpeechRules(settingsSpeechRulesText())", window)
        self.assertIn("function settingsVoicePath", window)
        self.assertIn("root.petController.setVoiceRuntimeSettings", window)
        self.assertIn("referenceAudioPath", window)
        self.assertIn("voiceTrainingArtifactPath", window)
        self.assertIn("function onSettingsChanged()", window)
        self.assertIn("property bool voiceReplyEnabled: true", window)
        self.assertIn("voiceReplyEnabled: root.voiceReplyEnabled", window)
        self.assertIn(
            "onVoiceReplyToggled: function(value) { root.voiceReplyEnabled = value; root.syncControlWindowState() }",
            window,
        )
        self.assertIn("petChatWindowRef.voiceCallActive = root.voiceReplyEnabled", window)
        self.assertIn("onVoiceReplyEnabledChanged", window)
        self.assertIn("onVoiceChatRequested: function(active) { root.voiceReplyEnabled = active }", window)
        self.assertIn("property var petChatWindowRef: null", window)
        self.assertIn("property var petControlWindowRef: null", window)
        self.assertIn("property bool chatPositionPending: false", window)
        self.assertIn("PetControlWindow", window)
        self.assertIn('"agentController": root.agentController', window)
        self.assertIn('"petAssetSettings": root.petAssetSettings', window)
        self.assertIn("petChatWindowComponent.createObject(null", window)
        self.assertIn("PetChatWindow {", window)
        self.assertIn("onVoiceChatRequested: function(active) { root.voiceReplyEnabled = active }", window)
        self.assertIn("scheduleChatWindowPosition()", window)
        self.assertIn("positionChatWindow()", window)
        self.assertIn("syncChatWindowState()", window)
        self.assertIn("onControlsRequested: root.openControlWindow()", window)
        self.assertIn("id: petDragHandler", scene)
        self.assertIn("acceptedButtons: Qt.LeftButton", scene)
        self.assertIn("acceptedButtons: Qt.RightButton", scene)
        self.assertNotIn("scale: root.scaleFactor", scene)
        self.assertNotIn("transformOrigin: Item.Bottom", scene)
        self.assertIn("propagateComposedEvents: true", scene)
        self.assertIn("mouse.accepted = false", scene)
        self.assertIn("signal dragRequested", scene)
        self.assertIn("signal controlsRequested", scene)
        self.assertIn("root.controlsRequested(mouse.x, mouse.y)", scene)
        self.assertNotIn("actionPopup.open()", scene)
        self.assertNotIn("property bool chatPanelOpen", scene)
        self.assertNotIn("function openChatPanel", scene)
        self.assertIn('petAudioLoader.item.playbackState = "stopped"', scene)
        self.assertIn('petAudioLoader.item.sourceUrl = ""', scene)
        self.assertIn("petAudioLoader.item.playbackState = root.petController.audioState", scene)
        self.assertIn("speechBubbleVisible", scene)
        self.assertIn("id: speechBubble", scene)
        self.assertIn("visible: root.speechBubbleTranslation.length > 0", scene)
        self.assertIn("width: Math.min(bubbleMaxWidth, 180)", scene)
        self.assertIn("height: 72", scene)
        self.assertIn("readonly property int speechBubbleSafeHeight: 84", scene)
        self.assertIn("anchors.topMargin: root.speechBubbleSafeHeight", scene)
        self.assertNotIn("speechBubbleReservedHeight", scene)
        self.assertNotIn("anchors.topMargin: root.speechBubbleReservedHeight", scene)
        self.assertIn("Qt.rgba(1.0, 0.96, 0.98, 0.96)", scene)
        self.assertNotIn("audioPlaybackActive", scene)
        self.assertNotIn("capturePaused", scene)
        self.assertIn("font.bold: true", scene)
        self.assertIn("property var petAssetSettings: null", scene)
        self.assertIn('modelRoot: root.petAssetSettings ? root.petAssetSettings.modelRoot : ""', scene)
        self.assertIn('modelJson: root.petAssetSettings ? root.petAssetSettings.modelJson : ""', scene)
        self.assertNotIn("id: topControls", scene)
        self.assertNotIn("id: dockControls", scene)
        self.assertNotIn("Popup", scene)
        self.assertIn("qml/pet/PetWindow.qml", qrc)
        self.assertIn("qml/pet/PetWindowRuntime.js", qrc)
        self.assertIn("qml/pet/PetScene.qml", qrc)
        self.assertIn("qml/pet/PetControlWindow.qml", qrc)
        self.assertIn("qml/pet/PetControlRuntime.js", qrc)
        self.assertIn("qml/pet/PetControlLive2DActionPanel.qml", qrc)
        self.assertIn("qml/pet/PetControlApiProviderPanel.qml", qrc)
        self.assertIn("qml/pet/PetChatWindow.qml", qrc)
        self.assertIn("qml/pet/PetAudioPlayer.qml", qrc)
        self.assertIn("qml/pet/Live2DCharacterPane.qml", qrc)
        self.assertIn('alias="icons/modelive2d.png"', qrc)

    def test_pet_control_window_contains_api_access_controls(self):
        panel = read_texts(
            PET_CONTROL_WINDOW_QML,
            PET_CONTROL_RUNTIME_JS,
            PET_CONTROL_LIVE2D_ACTION_PANEL_QML,
            PET_CONTROL_API_PROVIDER_PANEL_QML,
        )
        header = PET_CONTROL_HEADER_QML.read_text(encoding="utf-8")

        self.assertIn("Window {", panel)
        self.assertIn("maximumWidth: 320", panel)
        self.assertIn("PetControlHeader", panel)
        self.assertIn("PetControlRuntime.displayStatus", panel)
        self.assertIn("PetControlLive2DActionPanel", panel)
        self.assertIn("PetControlApiProviderPanel", panel)
        self.assertIn("function actionKindLabel", panel)
        self.assertIn("function modelFullName", panel)
        self.assertIn("onCloseRequested: root.hide()", panel)
        self.assertIn("panelCloseButton", header)
        self.assertIn("onClicked: root.closeRequested()", header)
        self.assertIn('text: "AI API 接入"', panel)
        self.assertIn("property var agentController", panel)
        self.assertIn("root.agentController.registerApiProvider", panel)
        self.assertIn("root.agentController.refreshModelList", panel)
        self.assertIn("root.agentController.switchModel", panel)
        self.assertIn("onRegisterRequested", panel)
        self.assertIn("onRefreshRequested", panel)
        self.assertIn("onModelSelected", panel)
        self.assertIn("apiProviderStatus", panel)
        self.assertIn("availableModels", panel)

    def test_pet_chat_window_is_separate_and_uses_pet_controller(self):
        chat = read_texts(PET_CHAT_WINDOW_QML, PET_CHAT_RUNTIME_JS, PET_CHAT_MESSAGE_LIST_QML, PET_CHAT_COMPOSER_QML)

        self.assertIn("Window {", chat)
        self.assertIn("flags: chatWindowFlags()", chat)
        self.assertIn("Qt.FramelessWindowHint", chat)
        self.assertIn("function openChat", chat)
        self.assertIn("ListModel", chat)
        self.assertIn("ListView", chat)
        self.assertIn("ChatMessageDelegate", chat)
        self.assertIn("TextArea", chat)
        self.assertIn("CompactIconButton", chat)
        self.assertIn("function sendMessage", chat)
        self.assertIn("function sendPendingText", chat)
        self.assertIn("function appendOrUpdateAssistantMessage", chat)
        self.assertIn("function assistantEventSnapshot", chat)
        self.assertIn("PetChatRuntime.assistantEventSnapshot", chat)
        self.assertIn("assistantEvent.resolvedEventKey", chat)
        self.assertIn("pendingSendAlreadyAppended", chat)
        self.assertIn("function syncModelSelection", chat)
        self.assertIn("function syncReplyLanguage", chat)
        self.assertIn("function syncSpeechRules", chat)
        self.assertIn("function speechRulesText", chat)
        self.assertIn("voiceChatRequested", chat)
        self.assertIn("signal voiceChatRequested(bool active)", chat)
        self.assertIn("root.voiceChatRequested(root.voiceCallActive)", chat)
        self.assertIn("语音回复已开启", chat)
        self.assertIn("videoChatRequested", chat)
        self.assertIn("controllerAvailable: !!root.petController", chat)
        self.assertIn("enabled: root.controllerAvailable && !root.controllerBusy", chat)
        self.assertIn("root.petController.startSession()", chat)
        self.assertIn("root.petController.sendText(trimmed)", chat)
        self.assertIn("root.petController.setModelSelection", chat)
        self.assertIn("root.petController.setReplyLanguage", chat)
        self.assertIn("root.petController.setSpeechRules", chat)
        self.assertIn("root.petController.setVoiceRuntimeSettings", chat)
        self.assertIn("function syncVoiceRuntimeSettings", chat)
        self.assertIn("referenceAudioPath", chat)
        self.assertIn("voiceTrainingArtifactPath", chat)
        self.assertIn("senderName: messageDelegateRoot.senderName.length > 0", chat)
        self.assertIn("root.messageSenderName(messageDelegateRoot.outgoingMessage)", chat)
        self.assertIn("showOutgoingSenderName: true", chat)
        self.assertIn("translationText: messageDelegateRoot.translationText", chat)
        self.assertIn("pendingAssistantIndex", chat)
        self.assertIn("pendingAssistantTurnId", chat)
        self.assertIn("onControlEventReceived", chat)

    def test_chat_message_delegate_has_builtin_avatar_fallback_icon(self):
        delegate = CHAT_MESSAGE_DELEGATE_QML.read_text(encoding="utf-8")
        avatar = CHAT_MESSAGE_AVATAR_QML.read_text(encoding="utf-8")

        self.assertIn("MessageAvatar", delegate)
        self.assertIn('source: "qrc:/icons/user.png"', avatar)
        self.assertIn("avatarImage.status === Image.Ready", avatar)
        self.assertIn("property bool loadFailed: false", avatar)
        self.assertIn("source: loadFailed ? root.defaultAvatarSource : baseSource", avatar)

    def test_chat_composer_bar_exposes_linux_chinese_input_fallback(self):
        composer = CHAT_COMPOSER_BAR_QML.read_text(encoding="utf-8")

        self.assertIn("property var imeBridgeController: null", composer)
        self.assertIn("focusMessageInput", composer)
        self.assertIn("messageInput.forceActiveFocus()", composer)
        self.assertIn("preeditText", composer)
        self.assertIn("pendingDraftSync", composer)
        self.assertIn("pendingDraftBaseText", composer)
        self.assertIn("inputMethodUpdatePending", composer)
        self.assertIn("syncDraftTextFromBinding", composer)
        self.assertIn("applyDraftText", composer)
        self.assertIn("scheduleInputMethodUpdate", composer)
        self.assertIn("Qt.inputMethod.update(Qt.ImQueryAll)", composer)
        self.assertIn("onCursorPositionChanged", composer)
        self.assertIn("onCursorRectangleChanged", composer)
        self.assertIn("inputMethodHints: Qt.ImhMultiLine", composer)
        self.assertNotIn("ToolButton {\n                Layout.fillWidth: true", composer)
        self.assertNotIn("imeBridgeActive", composer)
        self.assertNotIn("preferWindowsImeBridge", composer)
        self.assertNotIn("openImeBridge", composer)
        self.assertNotIn("pinyinCandidateMap", composer)
        self.assertNotIn("currentPinyinToken", composer)
        self.assertNotIn("pinyinCandidates", composer)
        self.assertNotIn("commitPinyinCandidate", composer)
        self.assertNotIn("insertCommittedText", composer)
        self.assertNotIn("replaceDraftText", composer)
        self.assertNotIn("Qt.inputMethod.show()", composer)
        self.assertNotIn("if (root.preferWindowsImeBridge", composer)
        self.assertNotIn("windowsImeBridgeAvailable", composer)
        self.assertNotIn("windowsImeBridgeBusy", composer)
        self.assertNotIn("onWindowsImeTextCommitted", composer)
        self.assertNotIn("Qt.Key_Space", composer)
        self.assertNotIn("pinyinCandidateBar", composer)
        self.assertNotIn('text: "中"', composer)

    def test_live2d_role_page_can_request_pet_preview_from_both_entry_points(self):
        pane = CHARACTER_PANE_QML.read_text(encoding="utf-8")
        runtime = LIVE2D_CHARACTER_RUNTIME_JS.read_text(encoding="utf-8")
        pane_runtime = pane + runtime
        shell = CHAT_SHELL_PAGE_QML.read_text(encoding="utf-8")
        normal_face = CHAT_NORMAL_FACE_QML.read_text(encoding="utf-8")
        shell_content = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/ChatShellContent.qml").read_text(
            encoding="utf-8"
        )
        shell_sources = shell + normal_face + shell_content

        self.assertRegex(pane, r"\bsignal\s+petPreviewRequested\s*\(\s*var\s+petAssetSettings\s*\)")
        self.assertRegex(pane, r"\bfunction\s+requestPetPreview\s*\(")
        self.assertIn('text: "启动桌宠"', pane)
        self.assertIn("root.storeDraftToSettings()", pane)
        self.assertIn("root.petPreviewRequested(petAssetSettings.toVariantMap())", pane)
        self.assertIn("onClicked: root.requestPetPreview()", pane)

        self.assertRegex(shell, r"\bsignal\s+petPreviewRequested\s*\(\s*var\s+petAssetSettings\s*\)")
        self.assertIn("Live2DCharacterPane", shell_sources)
        self.assertIn("onPetPreviewRequested: function(petAssetSettings)", shell_sources)
        self.assertIn("root.petPreviewRequested(petAssetSettings)", shell_sources)
        self.assertIn("ready_for_gpt_sovits", pane_runtime)

        for path in (SHARED_MAIN_QML, LINUX_MAIN_QML):
            source = path.read_text(encoding="utf-8")
            self.assertIn("function openPetWindow(petAssetSettings)", source)
            self.assertIn("function ensurePetWindow(petAssetSettings)", source)
            self.assertIn("win.openPet()", source)
            self.assertIn("startupPetTimer", source)
            self.assertIn('"petAssetSettings": settings', source)
            self.assertIn('"agentController": controller.agentController', source)
            self.assertIn("petWindowRef.petAssetSettings = settings", source)
            self.assertIn("onPetPreviewRequested: function(petAssetSettings)", source)
            self.assertIn("root.openPetWindow(petAssetSettings)", source)

    def test_pet_autostart_is_disabled_by_default_and_only_starts_from_saved_setting(self):
        pane = CHARACTER_PANE_QML.read_text(encoding="utf-8")
        runtime = LIVE2D_CHARACTER_RUNTIME_JS.read_text(encoding="utf-8")
        pane_runtime = pane + runtime
        behavior_panel = LIVE2D_BEHAVIOR_MEMORY_PANEL_QML.read_text(encoding="utf-8")
        behavior_column = LIVE2D_CHARACTER_BEHAVIOR_COLUMN_QML.read_text(encoding="utf-8")

        self.assertRegex(pane, r"\bproperty\s+bool\s+autoStartPetOnClientStart\s*:\s*false\b")
        self.assertIn("autoStartPetOnClientStart = settings.autoStartPetOnClientStart", pane_runtime)
        self.assertIn("settings.autoStartPetOnClientStart = source.autoStartPetOnClientStart", pane_runtime)
        self.assertIn("Live2DBehaviorMemoryPanel", pane + behavior_column)
        self.assertIn("autoStartPetOnClientStart: root.autoStartPetOnClientStart", pane)
        self.assertIn(
            "onAutoStartPetOnClientStartEdited: function(checked) { root.autoStartPetOnClientStart = checked }", pane
        )
        self.assertIn('title: "打开客户端自启"', behavior_panel)
        self.assertIn("checked: root.autoStartPetOnClientStart", behavior_panel)

        for path in (SHARED_MAIN_QML, LINUX_MAIN_QML):
            with self.subTest(path=path):
                source = path.read_text(encoding="utf-8")
                self.assertIn("PetAssetSettings", source)
                self.assertIn("startupPetSettings.load()", source)
                shown_start = source.index("function finishAppWindowShown()")
                shown_end = source.index("function ensurePetWindow(petAssetSettings)")
                shown_block = source[shown_start:shown_end]
                completed_start = source.index("Component.onCompleted:")
                completed_end = source.index("Component.onDestruction:")
                completed_block = source[completed_start:completed_end]
                self.assertRegex(
                    shown_block,
                    r"if\s*\(\s*startupPetSettings\.autoStartPetOnClientStart\s*\)\s*\{\s*startupPetTimer\.start\s*\(\s*\)",
                    f"{path.name} should start the desktop pet after the chat page is shown",
                )
                self.assertNotIn("startupPetTimer.start()", completed_block)

    def test_live2d_resource_path_buttons_open_native_file_pickers(self):
        pane = CHARACTER_PANE_QML.read_text(encoding="utf-8")
        character_preview = LIVE2D_CHARACTER_PREVIEW_PANEL_QML.read_text(encoding="utf-8")
        resource_voice = LIVE2D_RESOURCE_VOICE_PANEL_QML.read_text(encoding="utf-8")
        resource_bundle = read_texts(
            CHARACTER_PANE_QML, LIVE2D_CHARACTER_PREVIEW_PANEL_QML, LIVE2D_RESOURCE_VOICE_PANEL_QML
        )

        for function_name in (
            "pickModelJson",
            "pickModelRootDirectory",
            "pickMotionDirectory",
            "pickExpressionDirectory",
            "pickVoiceDirectory",
            "pickDefaultVoice",
        ):
            self.assertRegex(pane, rf"\bfunction\s+{function_name}\s*\(")

        for token in (
            "petAssetSettings.pickLocalFilePath",
            "petAssetSettings.pickLocalDirectoryPath",
        ):
            self.assertIn(token, pane)

        for token in (
            "onModelJsonPickRequested: root.pickModelJson()",
            "onModelRootPickRequested: root.pickModelRootDirectory()",
            "onMotionDirectoryPickRequested: root.pickMotionDirectory()",
            "onExpressionDirectoryPickRequested: root.pickExpressionDirectory()",
            "onVoiceDirectoryPickRequested: root.pickVoiceDirectory()",
            "onDefaultVoicePickRequested: root.pickDefaultVoice()",
        ):
            self.assertIn(token, pane)

        for token in (
            "signal modelJsonPickRequested()",
            "signal modelRootPickRequested()",
            "onClicked: root.modelJsonPickRequested()",
            "onClicked: root.modelRootPickRequested()",
        ):
            self.assertIn(token, character_preview)

        for token in (
            "signal motionDirectoryPickRequested()",
            "signal expressionDirectoryPickRequested()",
            "signal voiceDirectoryPickRequested()",
            "signal defaultVoicePickRequested()",
            "onClicked: root.motionDirectoryPickRequested()",
            "onClicked: root.expressionDirectoryPickRequested()",
            "onClicked: root.voiceDirectoryPickRequested()",
            "onClicked: root.defaultVoicePickRequested()",
        ):
            self.assertIn(token, resource_voice)

        for token in (
            "root.pickModelJson()",
            "root.pickModelRootDirectory()",
            "root.pickMotionDirectory()",
            "root.pickExpressionDirectory()",
            "root.pickVoiceDirectory()",
            "root.pickDefaultVoice()",
        ):
            self.assertIn(token, resource_bundle)

    def test_pet_controller_exposes_voice_training_submission_contract(self):
        header = PET_CONTROLLER_H.read_text(encoding="utf-8")
        source = read_texts(PET_CONTROLLER_CPP, PET_CONTROLLER_NETWORK_CPP, PET_CONTROLLER_VOICE_TRAINING_CPP)
        shell = CHAT_SHELL_PAGE_QML.read_text(encoding="utf-8")
        normal_face = CHAT_NORMAL_FACE_QML.read_text(encoding="utf-8")
        shell_content = (REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/ChatShellContent.qml").read_text(
            encoding="utf-8"
        )

        for token in (
            "voiceTrainingBusy",
            "voiceTrainingJobId",
            "voiceTrainingStatus",
            "voiceTrainingStage",
            "voiceTrainingProgress",
            "voiceTrainingArtifactPath",
            "voiceTrainingMessage",
            "Q_INVOKABLE void startVoiceTraining",
            "Q_INVOKABLE void refreshVoiceTrainingJob",
            "voiceTrainingChanged",
        ):
            self.assertIn(token, header)

        for token in (
            "/voice-training/jobs",
            "voice_training_create",
            "voice_training_get",
            "consent_confirmed",
            "reference_audio_path",
            "reference_audio_base64",
            "reference_audio_transfer",
            "applyVoiceTrainingJob",
        ):
            self.assertIn(token, source)

        self.assertIn("petController: controller.petController", shell + normal_face + shell_content)


if __name__ == "__main__":
    unittest.main()
