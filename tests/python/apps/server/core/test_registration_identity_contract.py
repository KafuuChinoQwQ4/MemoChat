import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps/server/core"
GATE_ACCOUNT_DAO = SERVER_CORE / "GateShared/core/persistence/PostgresDaoAccount.cpp"
GATE_H2_SUPPORT = SERVER_CORE / "GateShared/transports/h2/support"
SCHEMA_FILES = (
    REPO_ROOT / "apps/server/migrations/postgresql/business/001_baseline.sql",
    REPO_ROOT / "infra/deploy/local/init/postgresql/001-business.sql",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1 : index]
    raise AssertionError(f"Could not parse function body for {signature}")


def compact(text: str) -> str:
    return re.sub(r"\s+", " ", text)


class RegistrationIdentityContractTests(unittest.TestCase):
    def test_gate_registration_checks_duplicate_email_only(self):
        self.assertTrue(GATE_ACCOUNT_DAO.exists(), "GateServerCore account DAO split source should exist")
        source = read(GATE_ACCOUNT_DAO)
        body = compact(function_body(source, "int PostgresDao::RegUserTransaction"))

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', body)
        self.assertNotIn("OR name", body)
        self.assertNotRegex(body, r"WHERE\s+email\s*=\s*\$1\s+OR\s+name")

    def test_gate_reset_password_uses_unique_email_not_nickname(self):
        self.assertTrue(GATE_ACCOUNT_DAO.exists(), "GateServerCore account DAO split source should exist")
        dao = read(GATE_ACCOUNT_DAO)
        check_body = compact(function_body(dao, "bool PostgresDao::CheckEmail"))
        update_body = compact(function_body(dao, "bool PostgresDao::UpdatePwd"))
        h1_auth = read(SERVER_CORE / "AccountShared/domain/services/auth/AuthService.cpp")
        h1_account = read(SERVER_CORE / "AccountShared/domain/services/account/AccountPersistence.cpp")
        h2 = read(GATE_H2_SUPPORT / "Http2AuthSupport.cpp")

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', check_body)
        self.assertNotIn("WHERE name = $1", check_body)
        self.assertIn('UPDATE \\"user\\" SET pwd = $1 WHERE email = $2', update_body)
        self.assertNotIn("SET pwd = $1 WHERE name = $2", update_body)
        self.assertIn("UpdatePassword(email, pwd)", h1_auth)
        self.assertIn("UpdatePwd(email, password)", h1_account)
        self.assertIn('&AuthService::HandleResetPwd', h2)
        self.assertNotIn("UpdatePwd(email, pwd)", h2)

    def test_chatserver_postgres_registration_checks_duplicate_email_only(self):
        source = read(SERVER_CORE / "ChatServer/persistence/PostgresDaoUsers.cpp")
        body = compact(function_body(source, "int PostgresDao::RegUser"))
        postgres_branch = body.split("auto con = pool_->getConnection()", 1)[0]

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', postgres_branch)
        self.assertNotIn("OR name", postgres_branch)
        self.assertNotRegex(postgres_branch, r"WHERE\s+email\s*=\s*\$1\s+OR\s+name")

    def test_chatserver_reset_password_uses_unique_email_not_nickname(self):
        source = read(SERVER_CORE / "ChatServer/persistence/PostgresDaoUsers.cpp")
        check_body = compact(function_body(source, "bool PostgresDao::CheckEmail"))
        update_body = compact(function_body(source, "bool PostgresDao::UpdatePwd"))
        postgres_check = check_body.split("auto con = pool_->getConnection()", 1)[0]
        postgres_update = update_body.split("auto con = pool_->getConnection()", 1)[0]

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', postgres_check)
        self.assertNotIn("WHERE name = $1", postgres_check)
        self.assertIn('UPDATE \\"user\\" SET pwd = $1 WHERE email = $2', postgres_update)
        self.assertNotIn("SET pwd = $1 WHERE name = $2", postgres_update)

    def test_user_schema_keeps_email_unique_and_nickname_non_unique(self):
        for path in SCHEMA_FILES:
            with self.subTest(path=path.relative_to(REPO_ROOT)):
                schema = read(path)
                self.assertIn("CONSTRAINT uq_user_email UNIQUE (email)", schema)
                self.assertIn('CREATE INDEX IF NOT EXISTS idx_user_name ON "user"(name);', schema)
                self.assertNotRegex(schema, r"UNIQUE\s*\(\s*name\s*\)")
                self.assertNotRegex(schema, r"UNIQUE\s*\(\s*nick\s*\)")
                self.assertNotIn("uq_user_name", schema)
                self.assertNotIn("uq_user_nick", schema)


if __name__ == "__main__":
    unittest.main()
