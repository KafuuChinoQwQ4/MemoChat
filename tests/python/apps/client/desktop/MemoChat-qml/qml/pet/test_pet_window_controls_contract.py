import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
PET_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetWindow.qml"
PET_WINDOW_RUNTIME_JS = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetWindowRuntime.js"
PET_SCENE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetScene.qml"
PET_CONTROL_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlWindow.qml"
PET_CONTROL_HEADER_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlHeader.qml"
PET_CONTROL_API_PROVIDER_PANEL_QML = (
    REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetControlApiProviderPanel.qml"
)
PET_CHAT_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetChatWindow.qml"
PET_CHAT_MESSAGE_LIST_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetChatMessageList.qml"
PET_CHAT_COMPOSER_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetChatComposer.qml"

WINDOW_CONTROL_PROPERTIES = (
    "alwaysOnTop",
    "clickThrough",
    "decorativeMode",
    "debugPanelVisible",
    "scaleFactor",
    "micMuted",
    "cameraEnabled",
    "cloudVisionEnabled",
    "localOnlyMode",
    "debugRetentionEnabled",
    "voiceReplyEnabled",
    "providerAvailable",
)

SCENE_CONTROL_SIGNALS = (
    "controlsRequested",
    "dragRequested",
)

PANEL_CONTROL_SIGNALS = (
    "closePetRequested",
    "chatRequested",
    "resetPositionRequested",
    "alwaysOnTopToggled",
    "clickThroughToggled",
    "debugToggled",
    "scaleRequested",
    "micMuteToggled",
    "cameraToggled",
    "cloudVisionToggled",
    "localOnlyModeToggled",
    "debugRetentionToggled",
    "voiceReplyToggled",
)


def read_qml(path):
    return path.read_text(encoding="utf-8")


def function_body(source, name):
    match = re.search(r"\bfunction\s+" + re.escape(name) + r"\s*\([^)]*\)\s*\{", source)
    if not match:
        return ""

    depth = 1
    index = match.end()
    while index < len(source) and depth > 0:
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        index += 1
    return source[match.end() : index - 1]


class PetWindowControlsContractTests(unittest.TestCase):
    def test_pet_window_exposes_control_state_and_helpers(self):
        window = read_qml(PET_WINDOW_QML)

        for prop in WINDOW_CONTROL_PROPERTIES:
            self.assertRegex(window, r"\bproperty\s+\w+\s+" + re.escape(prop) + r"\b")

        for helper in (
            "openPet",
            "openPetChat",
            "ensureControlWindow",
            "openControlWindow",
            "positionControlWindow",
            "scheduleControlWindowPosition",
            "beginScaleInteraction",
            "endScaleInteraction",
            "applyScale",
            "resetPosition",
            "applyWindowFlags",
        ):
            self.assertRegex(window, r"\bfunction\s+" + re.escape(helper) + r"\s*\(")

    def test_pet_window_apply_flags_uses_additive_control_flags(self):
        window = read_qml(PET_WINDOW_QML)
        runtime = read_qml(PET_WINDOW_RUNTIME_JS)
        body = function_body(window, "applyWindowFlags") + "\n" + function_body(runtime, "petWindowFlags")
        self.assertTrue(body, "PetWindow.qml must define applyWindowFlags()")

        for token in (
            "Window",
            "Tool",
            "FramelessWindowHint",
            "WindowStaysOnTopHint",
            "WindowTransparentForInput",
        ):
            self.assertRegex(body, rf"\b(?:Qt|qt)\.{token}\b")

        self.assertIn('Qt.platform.os === "linux"', body)
        self.assertRegex(body, r"\balwaysOnTop\b")
        self.assertRegex(body, r"\bclickThrough\b")

    def test_pet_window_owns_independent_chat_window(self):
        window = read_qml(PET_WINDOW_QML)

        self.assertIn("property var petChatWindowRef: null", window)
        self.assertIn("petChatWindowComponent.createObject(null", window)
        self.assertIn("PetChatWindow {", window)
        self.assertIn("onVoiceChatRequested: function(active) { root.voiceReplyEnabled = active }", window)
        self.assertIn("petChatWindowRef.voiceCallActive = root.voiceReplyEnabled", window)
        self.assertIn("petChatWindowRef.openChat()", window)
        self.assertIn("onChatRequested: root.openPetChat()", window)
        self.assertIn("property bool chatPositionPending: false", window)
        self.assertIn("scheduleChatWindowPosition()", window)
        self.assertIn("positionChatWindow()", window)
        self.assertIn("syncChatWindowState()", window)
        self.assertIn('"petAssetSettings": root.petAssetSettings', window)

    def test_pet_window_owns_independent_control_window(self):
        window = read_qml(PET_WINDOW_QML)
        contract = window + "\n" + read_qml(PET_WINDOW_RUNTIME_JS)

        self.assertIn("property var petControlWindowRef: null", window)
        self.assertIn("petControlWindowComponent.createObject(null", window)
        self.assertIn("PetControlWindow", window)
        self.assertIn("petControlWindowRef.destroy()", window)
        self.assertIn("root.openControlWindow()", window)
        self.assertIn("positionControlWindow()", window)
        self.assertIn("panel.x =", window)
        self.assertIn("panel.y =", window)
        self.assertIn("rightSpace", contract)
        self.assertIn("leftSpace", contract)
        self.assertIn("rightFits", contract)
        self.assertIn("leftFits", contract)
        self.assertIn("scaleInteractionActive", window)
        self.assertIn("panelPositionPending", window)

    def test_pet_window_recovers_interactive_mode_when_reopened(self):
        window = read_qml(PET_WINDOW_QML)
        body = function_body(window, "openPet")
        self.assertTrue(body, "PetWindow.qml must define openPet()")

        self.assertRegex(body, r"\bclickThrough\s*=\s*false\b")
        self.assertRegex(body, r"\bdecorativeMode\s*=\s*false\b")
        self.assertRegex(window, r"\bdecorativeMode\s*=\s*clickThrough\b")

    def test_pet_window_wires_control_state_into_scene(self):
        window = read_qml(PET_WINDOW_QML)

        self.assertIn("PetScene", window)
        for prop in (
            "clickThrough",
            "decorativeMode",
            "scaleFactor",
            "cameraEnabled",
            "cloudVisionEnabled",
            "voiceReplyEnabled",
        ):
            self.assertRegex(window, re.escape(prop) + r"\s*:\s*root\." + re.escape(prop))

        for signal_name in SCENE_CONTROL_SIGNALS:
            handler = "on" + signal_name[0].upper() + signal_name[1:]
            self.assertIn(handler, window)

        for signal_name in PANEL_CONTROL_SIGNALS:
            handler = "on" + signal_name[0].upper() + signal_name[1:]
            self.assertIn(handler, window)

    def test_pet_scene_declares_control_inputs_and_signals(self):
        scene = read_qml(PET_SCENE_QML)

        for prop in (
            "clickThrough",
            "decorativeMode",
            "scaleFactor",
            "cameraEnabled",
            "cloudVisionEnabled",
            "voiceReplyEnabled",
        ):
            self.assertRegex(scene, r"\bproperty\s+\w+\s+" + re.escape(prop) + r"\b")

        expected_signatures = {
            "dragRequested": r"\bsignal\s+dragRequested\s*\(\s*\)",
            "controlsRequested": r"\bsignal\s+controlsRequested\s*\(\s*real\b",
        }
        for name, pattern in expected_signatures.items():
            self.assertRegex(scene, pattern, name)

    def test_pet_scene_only_requests_external_controls(self):
        scene = read_qml(PET_SCENE_QML)

        self.assertIn("id: petDragHandler", scene)
        self.assertIn("acceptedButtons: Qt.LeftButton", scene)
        self.assertIn("acceptedButtons: Qt.RightButton", scene)
        self.assertIn("propagateComposedEvents: true", scene)
        self.assertIn("mouse.accepted = false", scene)
        self.assertIn("root.dragRequested()", scene)
        self.assertIn("root.controlsRequested(mouse.x, mouse.y)", scene)
        self.assertNotIn("Popup", scene)
        self.assertNotIn("Slider", scene)

        self.assertIn("voiceReplyEnabled", scene)
        self.assertIn("PetAudioPlayer.qml", scene)
        self.assertIn('petAudioLoader.item.sourceUrl = ""', scene)
        self.assertIn('petAudioLoader.item.playbackState = "stopped"', scene)
        self.assertIn("active: root.voiceReplyEnabled && root.petController", scene)
        self.assertIn("speechBubbleText", scene)
        self.assertIn("speechBubbleTranslation", scene)
        self.assertIn("id: speechBubble", scene)
        self.assertIn("width: Math.min(bubbleMaxWidth, 180)", scene)
        self.assertIn("height: 72", scene)
        self.assertIn("readonly property int speechBubbleSafeHeight: 84", scene)
        self.assertIn("anchors.topMargin: root.speechBubbleSafeHeight", scene)
        self.assertIn("visible: root.speechBubbleTranslation.length > 0", scene)
        self.assertNotIn("speechBubbleReservedHeight", scene)
        self.assertNotIn("anchors.topMargin: root.speechBubbleReservedHeight", scene)
        self.assertNotIn("id: actionPopup", scene)
        self.assertNotRegex(scene, r"\bid\s*:\s*topControls\b")
        self.assertNotRegex(scene, r"\bid\s*:\s*dockControls\b")
        self.assertNotRegex(scene, r"\bid\s*:\s*composer\b")
        self.assertNotIn("speechPlaybackText", scene)
        self.assertNotIn("textToSpeechFallbackEnabled", scene)

    def test_pet_control_window_contains_fixed_side_panel_controls_and_api_access(self):
        panel = "\n".join(
            read_qml(path)
            for path in (
                PET_CONTROL_WINDOW_QML,
                PET_CONTROL_HEADER_QML,
                PET_CONTROL_API_PROVIDER_PANEL_QML,
            )
        )

        self.assertIn("Window {", panel)
        self.assertIn("maximumWidth: 320", panel)
        self.assertIn("function openPanel", panel)
        self.assertIn("panelCloseButton", panel)
        self.assertIn("onCloseRequested: root.hide()", panel)
        self.assertIn("onClicked: root.closeRequested()", panel)
        self.assertIn("Slider", panel)
        self.assertIn("scaleInteractionStarted", panel)
        self.assertIn("scaleInteractionFinished", panel)
        self.assertIn("onPressedChanged", panel)
        self.assertIn("live: false", panel)
        self.assertIn("scaleRequested(value)", panel)
        self.assertIn('text: "聊天"', panel)
        self.assertIn('text: "语音回复"', panel)
        self.assertIn('text: "AI API 接入"', panel)
        self.assertNotIn('text: "陪我整理"', panel)
        self.assertNotIn('text: "清除气泡"', panel)
        self.assertNotIn('root.sendQuickText("陪我整理一下现在的思路")', panel)
        self.assertNotIn("root.petController.clearSpeech()", panel)
        self.assertIn("property var agentController", panel)
        self.assertIn("registerApiProvider", panel)
        self.assertIn("root.agentController.registerApiProvider", panel)
        self.assertIn("root.agentController.refreshModelList", panel)
        self.assertIn("root.agentController.switchModel", panel)
        self.assertIn("apiProviderStatus", panel)
        self.assertIn("availableModels", panel)

    def test_pet_chat_window_sends_text_and_minimizes_independently(self):
        chat = "\n".join(
            read_qml(path)
            for path in (
                PET_CHAT_WINDOW_QML,
                PET_CHAT_MESSAGE_LIST_QML,
                PET_CHAT_COMPOSER_QML,
            )
        )

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
        self.assertIn("voiceChatRequested", chat)
        self.assertIn("videoChatRequested", chat)
        self.assertIn("messageInput.forceActiveFocus()", chat)
        self.assertIn("onWindowsImeBridgeChanged", chat)
        self.assertIn("root.petController.startSession()", chat)
        self.assertIn("root.petController.sendText(trimmed)", chat)
        self.assertIn("root.petController.setModelSelection", chat)
        self.assertIn("root.petController.setReplyLanguage", chat)


if __name__ == "__main__":
    unittest.main()
