import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
QML_ROOT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml"
QRC_ROOT = REPO_ROOT / "apps/client/desktop/MemoChat-qml/resources/qrc"
QML_QRCS = (
    QRC_ROOT / "qml-chat.qrc",
    QRC_ROOT / "qml-agent.qrc",
    QRC_ROOT / "qml-pet.qrc",
    QRC_ROOT / "qml-r18.qrc",
)

LIVE2D_CHARACTER_PANE = QML_ROOT / "pet/Live2DCharacterPane.qml"
LIVE2D_ASSET_COLUMN = QML_ROOT / "pet/Live2DCharacterAssetColumn.qml"
LIVE2D_BEHAVIOR_COLUMN = QML_ROOT / "pet/Live2DCharacterBehaviorColumn.qml"
LIVE2D_PROMPT_COLUMN = QML_ROOT / "pet/Live2DCharacterPromptColumn.qml"
AGENT_GAME_PANE = QML_ROOT / "agent/AgentGamePane.qml"
AGENT_GAME_SETUP_PANE = QML_ROOT / "agent/AgentGameSetupPane.qml"
AGENT_GAME_ROOM_PANE = QML_ROOT / "agent/AgentGameRoomPane.qml"
AGENT_GAME_TEMPLATE_PANE = QML_ROOT / "agent/AgentGameTemplatePane.qml"
AGENT_GAME_AGENT_CARD = QML_ROOT / "agent/AgentGameAgentCard.qml"
AGENT_GAME_OPTION_COMBO = QML_ROOT / "agent/AgentGameOptionCombo.qml"
AGENT_MARKDOWN_TEXT = QML_ROOT / "agent/AgentMarkdownText.qml"
AGENT_MARKDOWN_RUNTIME = QML_ROOT / "agent/AgentMarkdownRuntime.js"
AGENT_MARKDOWN_CODE_BLOCK = QML_ROOT / "agent/AgentMarkdownCodeBlock.qml"
R18_SOURCE_MANAGER_PANE = QML_ROOT / "r18/R18SourceManagerPane.qml"
R18_SOURCE_IMPORT_PANE = QML_ROOT / "r18/R18SourceImportPane.qml"
R18_OFFICIAL_SOURCE_CATALOG_PANE = QML_ROOT / "r18/R18OfficialSourceCatalogPane.qml"
R18_SOURCE_LIST_PANE = QML_ROOT / "r18/R18SourceListPane.qml"
R18_SHELL_PANE = QML_ROOT / "r18/R18ShellPane.qml"
R18_HOME_PANE = QML_ROOT / "r18/R18HomePane.qml"
CHAT_MESSAGE_DELEGATE = QML_ROOT / "chat/conversation/ChatMessageDelegate.qml"
CHAT_MESSAGE_STATUS_BADGE = QML_ROOT / "chat/conversation/ChatMessageStatusBadge.qml"
CHAT_MESSAGE_ACTION_MENU = QML_ROOT / "chat/conversation/ChatMessageActionMenu.qml"
CHAT_SMART_ACTION_POPUPS = QML_ROOT / "chat/conversation/ChatSmartActionPopups.qml"
CHAT_SMART_SUMMARY_POPUP = QML_ROOT / "chat/conversation/ChatSmartSummaryPopup.qml"
CHAT_SMART_TRANSLATE_POPUP = QML_ROOT / "chat/conversation/ChatSmartTranslatePopup.qml"


class LargePaneBoundaryTests(unittest.TestCase):
    def read(self, path):
        return path.read_text(encoding="utf-8")

    def qrc_text(self):
        return "\n".join(self.read(path) for path in QML_QRCS)

    def test_live2d_character_columns_exist_and_are_registered(self):
        qrc = self.qrc_text()
        for path in (LIVE2D_ASSET_COLUMN, LIVE2D_BEHAVIOR_COLUMN, LIVE2D_PROMPT_COLUMN):
            with self.subTest(path=path):
                self.assertTrue(path.is_file())
                self.assertIn(f"qml/{path.relative_to(QML_ROOT).as_posix()}", qrc)

    def test_live2d_character_pane_uses_column_boundaries(self):
        pane = self.read(LIVE2D_CHARACTER_PANE)

        self.assertIn("Live2DCharacterAssetColumn", pane)
        self.assertIn("Live2DCharacterBehaviorColumn", pane)
        self.assertIn("Live2DCharacterPromptColumn", pane)

        for token in (
            "Live2DCharacterPreviewPanel",
            "Live2DResourceVoicePanel",
            "Live2DPersonaPanel",
            "Live2DSpeechStylePanel",
            "Live2DBehaviorMemoryPanel",
            "Live2DPromptPreviewPanel",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, pane)

    def test_live2d_character_column_ownership(self):
        asset = self.read(LIVE2D_ASSET_COLUMN)
        behavior = self.read(LIVE2D_BEHAVIOR_COLUMN)
        prompt = self.read(LIVE2D_PROMPT_COLUMN)

        self.assertIn("Live2DCharacterPreviewPanel", asset)
        self.assertIn("Live2DResourceVoicePanel", asset)
        self.assertIn("Live2DPersonaPanel", behavior)
        self.assertIn("Live2DSpeechStylePanel", behavior)
        self.assertIn("Live2DBehaviorMemoryPanel", behavior)
        self.assertIn("Live2DPromptPreviewPanel", prompt)

    def test_agent_game_panes_exist_and_are_registered(self):
        qrc = self.qrc_text()
        for path in (AGENT_GAME_SETUP_PANE, AGENT_GAME_ROOM_PANE, AGENT_GAME_TEMPLATE_PANE):
            with self.subTest(path=path):
                self.assertTrue(path.is_file())
                self.assertIn(f"qml/{path.relative_to(QML_ROOT).as_posix()}", qrc)

    def test_agent_game_pane_uses_setup_boundary(self):
        pane = self.read(AGENT_GAME_PANE)

        self.assertIn("AgentGameSetupPane", pane)
        for token in (
            "AgentGameSetupHeader",
            "AgentGameRoomPane",
            "AgentGameTemplatePane",
            "AgentGameAgentCard",
            "roomTitleField",
            "rulesetCombo",
            "roleCombo",
            "hostEnabledCheck",
            "apiProviderNameField",
            "apiBaseUrlField",
            "apiKeyField",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, pane)

    def test_agent_game_child_pane_ownership(self):
        setup = self.read(AGENT_GAME_SETUP_PANE)
        room = self.read(AGENT_GAME_ROOM_PANE)
        template = self.read(AGENT_GAME_TEMPLATE_PANE)

        self.assertIn("AgentGameSetupHeader", setup)
        self.assertIn("AgentGameRoomPane", setup)
        self.assertIn("AgentGameTemplatePane", setup)
        self.assertIn("AgentGameAgentCard", setup)

        for token in ("roomTitleField", "rulesetCombo", "roleCombo", "hostEnabledCheck", "gameContentField"):
            with self.subTest(token=token):
                self.assertIn(token, room)

        for token in ("apiProviderNameField", "apiBaseUrlField", "apiKeyField", "registerRequested"):
            with self.subTest(token=token):
                self.assertIn(token, template)

    def test_agent_game_option_combo_boundary(self):
        qrc = self.qrc_text()
        card = self.read(AGENT_GAME_AGENT_CARD)

        self.assertTrue(AGENT_GAME_OPTION_COMBO.is_file())
        self.assertIn("qml/agent/AgentGameOptionCombo.qml", qrc)
        self.assertGreaterEqual(card.count("AgentGameOptionCombo"), 3)
        self.assertNotIn("delegate: ItemDelegate", card)

    def test_agent_game_option_combo_does_not_shadow_combobox_final_value(self):
        combo = self.read(AGENT_GAME_OPTION_COMBO)
        card = self.read(AGENT_GAME_AGENT_CARD)

        self.assertIn("property var selectedValue", combo)
        self.assertNotIn("property var currentValue", combo)
        self.assertIn("selectedValue:", card)
        self.assertNotIn("currentValue:", card)

    def test_agent_markdown_runtime_and_code_block_boundaries(self):
        qrc = self.qrc_text()
        markdown = self.read(AGENT_MARKDOWN_TEXT)

        for path in (AGENT_MARKDOWN_RUNTIME, AGENT_MARKDOWN_CODE_BLOCK):
            with self.subTest(path=path):
                self.assertTrue(path.is_file())
                self.assertIn(f"qml/{path.relative_to(QML_ROOT).as_posix()}", qrc)

        self.assertIn('import "AgentMarkdownRuntime.js" as AgentMarkdownRuntime', markdown)
        self.assertIn("AgentMarkdownCodeBlock", markdown)

        for token in (
            "function highlightLine",
            "function highlightedCode",
            "function keywordSource",
            "var cppWords",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, markdown)

    def test_r18_source_manager_panes_exist_and_are_registered(self):
        qrc = self.qrc_text()
        for path in (R18_SOURCE_IMPORT_PANE, R18_OFFICIAL_SOURCE_CATALOG_PANE, R18_SOURCE_LIST_PANE):
            with self.subTest(path=path):
                self.assertTrue(path.is_file())
                self.assertIn(f"qml/{path.relative_to(QML_ROOT).as_posix()}", qrc)

    def test_r18_source_manager_uses_child_boundaries(self):
        pane = self.read(R18_SOURCE_MANAGER_PANE)

        self.assertIn("R18SourceImportPane", pane)
        self.assertIn("R18OfficialSourceCatalogPane", pane)
        self.assertIn("R18SourceListPane", pane)
        for token in (
            "R18ImportedSourceRow",
            "R18OfficialSourceRow",
            "qrc:/icons/r18_datasource.png",
            "qrc:/icons/file.png",
            "sourceStatusText",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, pane)

    def test_r18_source_child_pane_ownership(self):
        import_pane = self.read(R18_SOURCE_IMPORT_PANE)
        official_pane = self.read(R18_OFFICIAL_SOURCE_CATALOG_PANE)
        list_pane = self.read(R18_SOURCE_LIST_PANE)

        self.assertIn("qrc:/icons/r18_datasource.png", import_pane)
        self.assertIn("sourceCatalogInputEdited", import_pane)
        self.assertIn("sourceCatalogPathRequested", import_pane)

        self.assertIn("qrc:/icons/file.png", official_pane)
        self.assertIn("R18OfficialSourceRow", official_pane)
        self.assertIn("officialSourceImportRequested", official_pane)

        self.assertIn("R18ImportedSourceRow", list_pane)
        self.assertIn("sourceStatusText", list_pane)
        self.assertIn("importedSourceOpenRequested", list_pane)

    def test_r18_home_pane_exists_and_is_registered(self):
        qrc = self.qrc_text()

        self.assertTrue(R18_HOME_PANE.is_file())
        self.assertIn("qml/r18/R18HomePane.qml", qrc)

    def test_r18_shell_uses_home_boundary(self):
        shell = self.read(R18_SHELL_PANE)

        self.assertIn("R18HomePane", shell)
        for token in (
            "id: homeSearchField",
            "R18HomeCard",
            "entryAction: modelData.action",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, shell)

    def test_r18_home_pane_owns_search_and_card_list(self):
        home = self.read(R18_HOME_PANE)

        for token in (
            "id: homeSearchField",
            "R18HomeCard",
            "searchRequested",
            "entryActivated",
            "importRequested",
        ):
            with self.subTest(token=token):
                self.assertIn(token, home)

    def test_chat_message_status_badge_exists_and_is_registered(self):
        qrc = self.qrc_text()

        self.assertTrue(CHAT_MESSAGE_STATUS_BADGE.is_file())
        self.assertIn("qml/chat/conversation/ChatMessageStatusBadge.qml", qrc)

    def test_chat_message_delegate_uses_status_badge_boundary(self):
        delegate = self.read(CHAT_MESSAGE_DELEGATE)
        badge = self.read(CHAT_MESSAGE_STATUS_BADGE) if CHAT_MESSAGE_STATUS_BADGE.is_file() else ""

        self.assertIn("ChatMessageStatusBadge", delegate)
        for token in (
            "发送中...",
            "发送失败",
            "离线待补投",
            "已撤回",
        ):
            with self.subTest(token=token):
                self.assertNotIn(token, delegate)
                self.assertIn(token, badge)

    def test_chat_message_action_menu_exists_and_is_registered(self):
        qrc = self.qrc_text()

        self.assertTrue(CHAT_MESSAGE_ACTION_MENU.is_file())
        self.assertIn("qml/chat/conversation/ChatMessageActionMenu.qml", qrc)

    def test_chat_message_delegate_uses_action_menu_boundary(self):
        delegate = self.read(CHAT_MESSAGE_DELEGATE)
        action_menu = self.read(CHAT_MESSAGE_ACTION_MENU) if CHAT_MESSAGE_ACTION_MENU.is_file() else ""

        self.assertIn("ChatMessageActionMenu", delegate)
        self.assertNotIn("id: menu", delegate)
        for token in (
            "MenuItem",
            "replyRequested",
            "mentionRequested",
            "forwardRequested",
            "editRequested",
            "revokeRequested",
            "translateRequested",
        ):
            with self.subTest(token=token):
                self.assertIn(token, action_menu)

    def test_chat_smart_summary_popup_exists_and_is_registered(self):
        qrc = self.qrc_text()

        self.assertTrue(CHAT_SMART_SUMMARY_POPUP.is_file())
        self.assertIn("qml/chat/conversation/ChatSmartSummaryPopup.qml", qrc)

    def test_chat_smart_action_popups_uses_summary_boundary(self):
        popups = self.read(CHAT_SMART_ACTION_POPUPS)
        summary = self.read(CHAT_SMART_SUMMARY_POPUP) if CHAT_SMART_SUMMARY_POPUP.is_file() else ""

        self.assertIn("ChatSmartSummaryPopup", popups)
        for token in ("summaryColumn", "TextEdit", "summaryStatus", "summaryContent"):
            with self.subTest(token=token):
                self.assertNotIn(token, popups)
                self.assertIn(token, summary)
        self.assertIn("closeRequested", summary)

    def test_chat_smart_translate_popup_exists_and_is_registered(self):
        qrc = self.qrc_text()

        self.assertTrue(CHAT_SMART_TRANSLATE_POPUP.is_file())
        self.assertIn("qml/chat/conversation/ChatSmartTranslatePopup.qml", qrc)

    def test_chat_smart_action_popups_uses_translate_boundary(self):
        popups = self.read(CHAT_SMART_ACTION_POPUPS)
        translate = self.read(CHAT_SMART_TRANSLATE_POPUP) if CHAT_SMART_TRANSLATE_POPUP.is_file() else ""
        translate_instance = popups.split("ChatSmartTranslatePopup {", 1)[1].split("Popup {", 1)[0]

        self.assertIn("ChatSmartTranslatePopup", popups)
        self.assertIn("openWithDefaults", popups)
        self.assertIn("popupAvailableWidth: root.width", translate_instance)
        self.assertNotIn("availableWidth: root.width", translate_instance)
        self.assertIn("property real popupAvailableWidth", translate)
        self.assertNotIn("property real availableWidth", translate)
        self.assertNotIn("root.availableWidth", translate)
        for token in ("翻译消息", "ComboBox", "translateSourceBox", "translateTargetBox"):
            with self.subTest(token=token):
                self.assertNotIn(token, popups)
                self.assertIn(token, translate)
        for token in ("pendingTranslateText", "translateConfirmed"):
            with self.subTest(token=token):
                self.assertIn(token, translate)


if __name__ == "__main__":
    unittest.main()
