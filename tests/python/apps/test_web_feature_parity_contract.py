import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
WEB_SRC = REPO_ROOT / "apps" / "web" / "src"


def read_text(path):
    return path.read_text(encoding="utf-8")


class WebFeatureParityContractTest(unittest.TestCase):
    def test_contact_shell_exposes_friend_apply_workflow(self):
        source = read_text(WEB_SRC / "features" / "contact" / "components" / "ContactShellContent.tsx")

        self.assertIn("添加好友", source)
        self.assertIn("ID_SEARCH_USER_REQ", source)
        self.assertIn("ID_ADD_FRIEND_REQ", source)
        self.assertIn("ID_AUTH_FRIEND_REQ", source)
        self.assertIn("ID_SEARCH_USER_RSP", source)

    def test_group_shell_exposes_create_group_workflow(self):
        source = read_text(WEB_SRC / "features" / "group" / "components" / "GroupShellContent.tsx")

        self.assertIn("建群", source)
        self.assertIn("创建群聊", source)
        self.assertIn("ID_CREATE_GROUP_REQ", source)
        self.assertIn("member_user_ids", read_text(WEB_SRC / "features" / "chat" / "api" / "chatApi.ts"))

    def test_moments_shell_exposes_publish_workflow(self):
        source = read_text(WEB_SRC / "features" / "moments" / "components" / "MomentsShellContent.tsx")

        self.assertIn("ENDPOINTS.momentsPublish", source)
        self.assertIn("朋友圈内容", source)
        self.assertIn('media_type: "text"', source)
        self.assertIn("queryClient.invalidateQueries", source)

    def test_conversation_shell_exposes_smart_chat_actions(self):
        source = read_text(WEB_SRC / "features" / "chat" / "components" / "conversation" / "ConversationPane.tsx")
        api_source = read_text(WEB_SRC / "features" / "chat" / "api" / "chatApi.ts")
        endpoint_source = read_text(WEB_SRC / "core" / "config" / "endpoints.ts")

        self.assertIn("runSmartAction", source)
        self.assertIn("摘要", source)
        self.assertIn("建议", source)
        self.assertIn("翻译", source)
        self.assertIn("runSmartFeature", api_source)
        self.assertIn("aiSmart", endpoint_source)


if __name__ == "__main__":
    unittest.main()
