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

        # gateserver split Phase 2b: the apply-list query no longer JOINs the
        # "user" table (that table moved to memo_account). Sender identity is now
        # resolved through the GetUsersByUids batch resolver (account-data seam),
        # but the sender ICON must still reach the bootstrap ApplyInfo.
        self.assertIn("GetUsersByUids(from_uids)", get_apply_list)
        self.assertNotRegex(
            get_apply_list,
            r'txn\.exec[^;]*JOIN\s+"?user"?',
            "GetApplyList must not pqxx-JOIN the user table after the account split",
        )
        # icon (and the rest of the sender identity) is carried from the resolver
        # into the ApplyInfo for the bootstrap payload.
        self.assertIn("->icon", get_apply_list)
        self.assertIn("user_public_id", get_apply_list)
        self.assertIn(
            'std::make_shared<ApplyInfo>(ar.uid, name, "", icon, nick, sex, ar.status, user_public_id)', get_apply_list
        )

    def test_apply_list_warmup_query_matches_icon_projection(self):
        source = POSTGRES_DAO.read_text(encoding="utf-8")
        warmup = extract_function(source, "void PostgresDao::WarmupRelationBootstrapQueries")

        # After the account split the warmup warms only relation tables; user
        # base-info is resolved separately via GetUsersByUids, so the warmup must
        # NOT JOIN "user" (it would break once user lives in memo_account).
        self.assertNotRegex(
            warmup,
            r'txn\.exec[^;]*JOIN\s+"?user"?',
            "warmup must not pqxx-JOIN the user table after the account split",
        )
        self.assertRegex(warmup, r"SELECT\s+a\.from_uid, a\.status\b")
        self.assertIn("FROM friend_apply AS a", warmup)

    def test_relation_bootstrap_serializes_apply_icon_to_client(self):
        source = CHAT_RELATION_SERVICE.read_text(encoding="utf-8")
        bootstrap = extract_function(source, "void ChatRelationService::AppendRelationBootstrapJson")

        self.assertIn("ChatOutput::ChatRelationApplyRowDto row;", bootstrap)
        self.assertIn("row.user_id = apply->_user_id;", bootstrap)
        self.assertIn("row.icon = apply->_icon;", bootstrap)
        self.assertIn("bootstrap.apply_list.push_back(row);", bootstrap)
        self.assertLess(
            bootstrap.index("row.icon = apply->_icon;"), bootstrap.index("bootstrap.apply_list.push_back(row);")
        )


if __name__ == "__main__":
    unittest.main()
