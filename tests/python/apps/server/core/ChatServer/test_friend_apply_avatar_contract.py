import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
CHATSERVER_DIR = REPO_ROOT / "apps/server/core/ChatServer"
POSTGRES_DAO_USERS = CHATSERVER_DIR / "persistence/PostgresDaoUsers.cpp"
POSTGRES_DAO = CHATSERVER_DIR / "persistence/PostgresDao.cpp"
CHAT_RELATION_SERVICE = CHATSERVER_DIR / "domain/relation/ChatRelationService.cpp"


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"Function body not found for {signature}")


class FriendApplyAvatarContractTests(unittest.TestCase):
    def test_apply_list_query_keeps_sender_icon_for_bootstrap(self):
        source = POSTGRES_DAO_USERS.read_text(encoding="utf-8")
        get_apply_list = extract_function(source, "bool PostgresDao::GetApplyList")

        self.assertIn("usr.user_id", get_apply_list)
        self.assertIn("usr.icon", get_apply_list)
        self.assertIn('row["icon"].is_null() ? "" : row["icon"].c_str()', get_apply_list)

        self.assertIn("user.nick, user.sex, user.user_id, user.icon", get_apply_list)
        self.assertIn('auto icon = res->isNull("icon") ? "" : res->getString("icon");', get_apply_list)
        self.assertIn('std::make_shared<ApplyInfo>(uid, name, "", icon, nick, sex, status, user_id)', get_apply_list)

    def test_apply_list_warmup_query_matches_icon_projection(self):
        source = POSTGRES_DAO.read_text(encoding="utf-8")
        warmup = extract_function(source, "void PostgresDao::WarmupRelationBootstrapQueries")

        self.assertRegex(warmup, r"SELECT\s+a\.from_uid, a\.status, u\.name, u\.nick, u\.sex, u\.user_id, u\.icon")
        self.assertIn("user.nick, user.sex, user.user_id, user.icon", warmup)

    def test_relation_bootstrap_serializes_apply_icon_to_client(self):
        source = CHAT_RELATION_SERVICE.read_text(encoding="utf-8")
        bootstrap = extract_function(source, "void ChatRelationService::AppendRelationBootstrapJson")

        self.assertIn('obj["icon"] = apply->_icon;', bootstrap)
        self.assertLess(
            bootstrap.index('obj["icon"] = apply->_icon;'), bootstrap.index('out["apply_list"].append(obj);')
        )


if __name__ == "__main__":
    unittest.main()
