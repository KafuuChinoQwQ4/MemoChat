import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps/server/core"
GATE_ACCOUNT_DAO = SERVER_CORE / "GateShared/core/persistence/PostgresDaoAccount.cpp"
CHAT_USERS_DAO = SERVER_CORE / "ChatServer/persistence/PostgresDaoUsers.cpp"
ACCOUNT_AUTH_SERVICE = SERVER_CORE / "AccountShared/domain/services/auth/AuthService.cpp"
ACCOUNT_PERSISTENCE = SERVER_CORE / "AccountShared/domain/services/account/AccountPersistence.cpp"
AUTH_PUBLIC_DTOS = SERVER_CORE / "AccountShared/core/support/AuthPublicDtos.hpp"
CLIENT_COMMON = REPO_ROOT / "apps/client/desktop/MemoChat-qml/core/common"
CLIENT_AUTH_CONTROLLER = REPO_ROOT / "apps/client/desktop/MemoChat-qml/features/auth/AuthController.cpp"
AUTH_SCRIPT_CONTRACT_FILES = (
    REPO_ROOT / "tools/loadtest/k6/http-login.js",
    REPO_ROOT / "tools/loadtest/k6/run-http-bench.ps1",
    REPO_ROOT / "tools/loadtest/python-loadtest/py_loadtest.py",
    REPO_ROOT / "tools/loadtest/python-loadtest/config.json",
    REPO_ROOT / "tools/loadtest/python-loadtest/config.benchmark.json",
    REPO_ROOT / "tools/scripts/dev/runtime_smoke_full_chat.py",
    REPO_ROOT / "tools/scripts/loadtest-accounts/seed_loadtest_accounts.py",
)
SCHEMA_FILES = (
    REPO_ROOT / "apps/server/migrations/postgresql/business/001_baseline.sql",
    REPO_ROOT / "infra/deploy/local/init/postgresql/001-business.sql",
)
PASSWORD_HASH_MIGRATION = REPO_ROOT / "apps/server/migrations/postgresql/business/010_password_hash.sql"


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


def struct_body(source: str, name: str) -> str:
    match = re.search(rf"struct\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}};", source, re.S)
    if match is None:
        raise AssertionError(f"Could not parse struct body for {name}")
    return match.group("body")


def compact(text: str) -> str:
    return re.sub(r"\s+", " ", text)


class RegistrationIdentityContractTests(unittest.TestCase):
    def test_client_auth_sends_current_password_strings(self):
        auth = read(CLIENT_AUTH_CONTROLLER)
        global_cpp = read(CLIENT_COMMON / "global.cpp")
        global_h = read(CLIENT_COMMON / "global.h")

        login_body = compact(function_body(auth, "void AuthController::sendLogin"))
        register_body = compact(function_body(auth, "void AuthController::sendRegister"))
        reset_body = compact(function_body(auth, "void AuthController::sendResetPassword"))

        self.assertIn('payload["passwd"] = password;', login_body)
        self.assertIn('payload["passwd"] = password;', register_body)
        self.assertIn('payload["confirm"] = confirm;', register_body)
        self.assertIn('payload["passwd"] = password;', reset_body)
        self.assertNotIn("xorString", auth)
        self.assertNotIn("xorString", global_cpp)
        self.assertNotIn("xorString", global_h)

    def test_register_and_reset_responses_do_not_return_password_fields(self):
        dto_header = read(AUTH_PUBLIC_DTOS)
        service = read(ACCOUNT_AUTH_SERVICE)

        for dto in ("AuthRegisterResponseDto", "AuthResetPasswordResponseDto", "AuthLoginUserProfileDto"):
            body = struct_body(dto_header, dto)
            with self.subTest(dto=dto):
                self.assertNotRegex(body, r"\b(passwd|confirm|pwd|password|password_hash)\b")

        register_body = function_body(service, "bool AuthService::HandleUserRegister")
        register_response = register_body[register_body.index("gateauthsupport::AuthRegisterResponseDto") :]
        reset_body = function_body(service, "bool AuthService::HandleResetPwd")
        reset_response = reset_body[reset_body.index("gateauthsupport::AuthResetPasswordResponseDto") :]

        for label, response_body in (("register", register_response), ("reset", reset_response)):
            with self.subTest(response=label):
                self.assertNotIn(".passwd", response_body)
                self.assertNotIn(".confirm", response_body)
                self.assertNotIn('root["passwd"]', response_body)
                self.assertNotIn('root["confirm"]', response_body)
                self.assertNotIn("password_hash", response_body)

    def test_auth_loadtest_and_smoke_scripts_use_current_password_strings(self):
        forbidden = (
            "xorString",
            "xor_encode",
            "xorEncode",
            "USE_XOR_PASSWD",
            "use_xor_passwd",
            "LegacyXor",
            "DecodeLegacyXor",
        )
        for path in AUTH_SCRIPT_CONTRACT_FILES:
            source = read(path)
            with self.subTest(path=path.relative_to(REPO_ROOT)):
                for token in forbidden:
                    self.assertNotIn(token, source)

    def test_gate_registration_checks_duplicate_email_only(self):
        self.assertTrue(GATE_ACCOUNT_DAO.exists(), "GateServerCore account DAO split source should exist")
        source = read(GATE_ACCOUNT_DAO)
        body = compact(function_body(source, "int PostgresDao::RegUserTransaction"))

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', body)
        self.assertNotIn("OR name", body)
        self.assertNotRegex(body, r"WHERE\s+email\s*=\s*\$1\s+OR\s+name")
        self.assertIn("memochat::auth::HashPassword(pwd, password_hash)", body)
        self.assertIn("uid, name, email, pwd, password_hash, nick, icon, user_id", body)
        self.assertIn("VALUES ($1, $2, $3, '', $4", body)
        self.assertIn("password_hash", body)
        self.assertNotIn("DecodeLegacyXorPwd", body)
        self.assertNotIn("pwd_stored", body)

    def test_gate_password_check_verifies_password_hash_only(self):
        self.assertTrue(GATE_ACCOUNT_DAO.exists(), "GateServerCore account DAO split source should exist")
        source = read(GATE_ACCOUNT_DAO)
        body = compact(function_body(source, "bool PostgresDao::CheckPwd"))

        self.assertIn("password_hash", body)
        self.assertIn("memochat::auth::VerifyPassword(password_hash, pwd)", body)
        self.assertIn("userInfo.pwd.clear()", body)
        self.assertNotIn("if (pwd != origin_pwd)", body)
        self.assertNotIn("origin_pwd", body)
        self.assertNotIn("DecodeLegacyXorPwd", body)
        self.assertNotIn("decoded_pwd", body)

    def test_gate_reset_password_uses_unique_email_not_nickname(self):
        self.assertTrue(GATE_ACCOUNT_DAO.exists(), "GateServerCore account DAO split source should exist")
        dao = read(GATE_ACCOUNT_DAO)
        check_body = compact(function_body(dao, "bool PostgresDao::CheckEmail"))
        update_body = compact(function_body(dao, "bool PostgresDao::UpdatePwd"))
        h1_auth = read(ACCOUNT_AUTH_SERVICE)
        h1_account = read(ACCOUNT_PERSISTENCE)

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', check_body)
        self.assertNotIn("WHERE name = $1", check_body)
        self.assertIn("memochat::auth::HashPassword(newpwd, password_hash)", update_body)
        self.assertIn("UPDATE \\\"user\\\" SET password_hash = $1, pwd = '' WHERE email = $2", update_body)
        self.assertNotIn("SET pwd = $1", update_body)
        self.assertNotIn("SET pwd = $1 WHERE name = $2", update_body)
        self.assertIn("UpdatePassword(email, pwd)", h1_auth)
        self.assertIn("UpdatePwd(email, password)", h1_account)

    def test_chatserver_postgres_registration_checks_duplicate_email_only(self):
        source = read(CHAT_USERS_DAO)
        body = compact(function_body(source, "int PostgresDao::RegUser"))
        postgres_branch = body.split("auto con = pool_->getConnection()", 1)[0]

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', postgres_branch)
        self.assertNotIn("OR name", postgres_branch)
        self.assertNotRegex(postgres_branch, r"WHERE\s+email\s*=\s*\$1\s+OR\s+name")
        self.assertIn("memochat::auth::HashPassword(pwd, password_hash)", body)
        self.assertIn("uid, name, email, pwd, password_hash, nick, icon, user_id", body)
        self.assertIn("VALUES ($1, $2, $3, '', $4", body)

    def test_chatserver_reset_password_uses_unique_email_not_nickname(self):
        source = read(CHAT_USERS_DAO)
        check_body = compact(function_body(source, "bool PostgresDao::CheckEmail"))
        update_body = compact(function_body(source, "bool PostgresDao::UpdatePwd"))
        postgres_check = check_body.split("auto con = pool_->getConnection()", 1)[0]
        postgres_update = update_body.split("auto con = pool_->getConnection()", 1)[0]

        self.assertIn('SELECT 1 FROM \\"user\\" WHERE email = $1 LIMIT 1', postgres_check)
        self.assertNotIn("WHERE name = $1", postgres_check)
        self.assertIn("memochat::auth::HashPassword(newpwd, password_hash)", postgres_update)
        self.assertIn("UPDATE \\\"user\\\" SET password_hash = $1, pwd = '' WHERE email = $2", postgres_update)
        self.assertNotIn("SET pwd = $1", postgres_update)
        self.assertNotIn("SET pwd = $1 WHERE name = $2", postgres_update)

    def test_chatserver_postgres_password_check_verifies_password_hash_only(self):
        source = read(CHAT_USERS_DAO)
        body = compact(function_body(source, "bool PostgresDao::CheckPwd"))
        postgres_branch = body.split("(void) name;", 1)[0]

        self.assertIn("password_hash", postgres_branch)
        self.assertIn("memochat::auth::VerifyPassword(password_hash, pwd)", postgres_branch)
        self.assertIn("userInfo.pwd.clear()", postgres_branch)
        self.assertNotIn("if (pwd != origin_pwd)", body)
        self.assertNotIn("origin_pwd", body)
        for removed_helper in (
            "DecodeLegacyXorPwd",
            "ShouldAttemptLegacyXorDecode",
            "IsPasswordAccepted",
            "legacy_decoded_password_match",
        ):
            self.assertNotIn(removed_helper, postgres_branch)

    def test_user_schema_keeps_email_unique_and_nickname_non_unique(self):
        for path in SCHEMA_FILES:
            with self.subTest(path=path.relative_to(REPO_ROOT)):
                schema = read(path)
                self.assertRegex(schema, r"\bpassword_hash\s+text\s+NOT NULL\s+DEFAULT\s+''")
                self.assertIn("CONSTRAINT uq_user_email UNIQUE (email)", schema)
                self.assertIn('CREATE INDEX IF NOT EXISTS idx_user_name ON "user"(name);', schema)
                self.assertNotRegex(schema, r"UNIQUE\s*\(\s*name\s*\)")
                self.assertNotRegex(schema, r"UNIQUE\s*\(\s*nick\s*\)")
                self.assertNotIn("uq_user_name", schema)
                self.assertNotIn("uq_user_nick", schema)

    def test_password_hash_migration_adds_forward_only_hash_column(self):
        migration = read(PASSWORD_HASH_MIGRATION)

        self.assertIn("ADD COLUMN IF NOT EXISTS password_hash text NOT NULL DEFAULT ''", migration)
        self.assertNotRegex(migration, r"\bCOALESCE\s*\(\s*password_hash\s*,\s*pwd\s*\)")
        self.assertIsNone(re.search(r"\bCASE\b.*\bpwd\b", migration, re.S))


if __name__ == "__main__":
    unittest.main()
