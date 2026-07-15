"""Account bounded-context contract.

Register, Login, and Account are three deployment ROLES of ONE account
bounded-context, not three autonomous data services. They share the
`memo_account` DB, the `memo_account_app` role, one migration set, and a single
data-access layer (AccountPersistence -> PostgresMgr -> PostgresDaoAccount).

The architecture review's finding #2 accepts the shared DB, but requires that it
stay explicitly "one domain / multiple roles" — the risk being that the three
processes drift into their own incompatible SQL against the same tables, or that
the one genuinely cross-role shared-write table (auth_refresh_token) loses the
row-lock serialization that keeps concurrent Login-rotate vs Account-revoke
correct.

These are static source/config scanners (no build, no live service), matching
the sibling contract tests. They assert the POSITIVE invariant of the current,
verified state, so they fail if a service starts issuing its own account SQL, if
a config stops pinning the shared DB, or if the shared-write serialization
discipline is removed. See docs/database-ownership.md ("账号域 bounded context").
"""

import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"

# The three deployment roles of the account bounded-context.
ACCOUNT_SERVICE_DIRS = {
    "RegisterService": SERVER_CORE / "RegisterService",
    "LoginService": SERVER_CORE / "LoginService",
    "AccountService": SERVER_CORE / "AccountService",
}
ACCOUNT_INIS = {
    "RegisterService": SERVER_CORE / "RegisterService" / "register.ini",
    "LoginService": SERVER_CORE / "LoginService" / "login.ini",
    "AccountService": SERVER_CORE / "AccountService" / "account.ini",
}

# The single shared data-access layer for the account domain.
ACCOUNT_DAL_CPP = SERVER_CORE / "AccountShared" / "domain" / "services" / "account" / "AccountPersistence.cpp"
ACCOUNT_SQL_UNIT = SERVER_CORE / "GateShared" / "core" / "persistence" / "PostgresDaoAccount.cpp"
DOC = SERVER_CORE / "docs" / "database-ownership.md"

# Markers that a translation unit issues its own SQL rather than delegating.
RAW_SQL_MARKERS = ("pqxx::", "exec_params", "exec0", "exec1", "INSERT INTO", "UPDATE ", 'FROM "user"')


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


class AccountServicesShareOneDalTest(unittest.TestCase):
    """The three roles never issue their own SQL — they share one DAL."""

    def test_no_account_service_dir_contains_raw_sql(self):
        offenders = []
        for service, root in ACCOUNT_SERVICE_DIRS.items():
            for cpp in root.rglob("*.cpp"):
                src = read(cpp)
                for marker in RAW_SQL_MARKERS:
                    if marker in src:
                        offenders.append(f"{service}:{cpp.relative_to(REPO_ROOT)} ({marker!r})")
        self.assertEqual(
            [],
            offenders,
            "Account services must route ALL account-table access through the shared "
            "AccountPersistence DAL, never their own SQL. Divergent SQL across the three "
            f"roles is the DB-level coupling finding #2 forbids: {offenders}",
        )

    def test_account_sql_is_confined_to_the_single_shared_unit(self):
        """The account tables' SQL lives in exactly one place. If it appears in a
        second file, the domain has grown a divergent SQL path."""
        self.assertTrue(ACCOUNT_SQL_UNIT.is_file())
        table_literals = ("auth_refresh_token", "RegUserTransaction")
        dal_src = read(ACCOUNT_SQL_UNIT)
        for literal in table_literals:
            with self.subTest(literal=literal):
                self.assertIn(literal, dal_src, f"{ACCOUNT_SQL_UNIT.name} must own the account SQL for {literal}.")


class AccountConfigIsOneDomainMultipleRolesTest(unittest.TestCase):
    """All three .ini pin the same DB+role and none is a cross-DB bridge —
    i.e. they OWN the table (same pool), they don't reach into someone else's."""

    def test_all_three_pin_shared_account_db_and_role(self):
        for service, ini in ACCOUNT_INIS.items():
            src = read(ini)
            with self.subTest(service=service):
                self.assertRegex(src, r"(?m)^\s*Database\s*=\s*memo_account\s*$")
                self.assertRegex(src, r"(?m)^\s*User\s*=\s*memo_account_app\s*$")

    def test_no_account_service_declares_a_cross_db_bridge(self):
        """A [AccountPostgres] bridge would mean the service is reaching into an
        account DB it does NOT own (that is ChatServer's legacy pattern, finding
        #3). The three account roles own memo_account directly, so they must not
        declare a bridge."""
        for service, ini in ACCOUNT_INIS.items():
            src = read(ini)
            with self.subTest(service=service):
                self.assertNotIn(
                    "[AccountPostgres]",
                    src,
                    f"{service} owns memo_account via its own [Postgres] pool; a cross-DB "
                    f"bridge here would confuse ownership with borrowing.",
                )


class AccountSharedWriteSerializationTest(unittest.TestCase):
    """auth_refresh_token is the one table written by TWO roles (Login rotate,
    Account revoke). Its correctness depends on row-lock serialization living in
    the single shared DAL — this locks that discipline in."""

    def test_refresh_token_table_touched_only_in_shared_dal(self):
        offenders = []
        for cpp in SERVER_CORE.rglob("*.cpp"):
            if cpp == ACCOUNT_SQL_UNIT:
                continue
            if "auth_refresh_token" in read(cpp):
                offenders.append(cpp.relative_to(REPO_ROOT).as_posix())
        self.assertEqual(
            [],
            offenders,
            "auth_refresh_token is a cross-role shared-write table; its SQL must stay in the "
            f"single shared DAL ({ACCOUNT_SQL_UNIT.name}) so the FOR UPDATE serialization "
            f"cannot be bypassed by an ad-hoc write elsewhere: {offenders}",
        )

    def test_refresh_token_rotation_and_revoke_hold_row_locks(self):
        """Both mutation paths on the shared-write table must take FOR UPDATE row
        locks — that is what serializes concurrent rotate-vs-revoke."""
        dal_src = read(ACCOUNT_SQL_UNIT)
        for_update_count = len(re.findall(r"FOR UPDATE", dal_src))
        self.assertGreaterEqual(
            for_update_count,
            2,
            "The refresh-token rotate and revoke paths must both SELECT ... FOR UPDATE. "
            f"Found {for_update_count} FOR UPDATE lock(s); dropping them reopens the "
            "concurrent Login-rotate vs Account-revoke race.",
        )


class AccountBoundedContextDocumentedTest(unittest.TestCase):
    """The 'one domain / multiple roles' model must be written down, not folklore."""

    def test_doc_states_bounded_context_and_shared_write_contract(self):
        self.assertTrue(DOC.is_file())
        doc = read(DOC)
        for token in ("bounded context", "auth_refresh_token", "FOR UPDATE"):
            with self.subTest(token=token):
                self.assertIn(token, doc, f"database-ownership.md must document {token!r}.")


if __name__ == "__main__":
    unittest.main()
