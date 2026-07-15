"""GateShared governance-principle contract tests.

Operationalizes the three governance principles the architecture review set for
the shared layer, so "GateShared 演化成新的 GateServer" drift fails CI instead of
being caught by review:

  1. 共享稳定机制，不共享具体业务策略
     (share stable MECHANISM, not concrete business POLICY)
  2. 共享接口和基础 DTO，不共享跨域事务
     (share interfaces + base DTOs, not cross-domain transactions)
  3. 服务专属代码必须留在所属服务目录
     (service-specific code stays in its owning service directory)

These are static source/CMake scanners (no build, no live service), matching the
sibling contract tests under this tree. They assert the POSITIVE invariant of
each principle, and — critically — RATCHET the one known, documented exception
(the wide fused PostgresDao) so it can only shrink, never grow: no NEW business
domain may be added to the shared DAO, and every domain that peels off must land
a service-local persistence seam. See docs/gateshared-layering.md.
"""

import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
GATE_SHARED = SERVER_CORE / "GateShared"
INFRA_CMAKE = GATE_SHARED / "core" / "CMakeLists.txt"
LAYERING_DOC = SERVER_CORE / "docs" / "gateshared-layering.md"

# Every business-domain route/support library defined by an owning service. The
# shared infra slices and the shared runtime framework must NEVER link these:
# dependency flows domain -> shared, never shared -> domain.
DOMAIN_TARGETS = (
    "GateAiDomain",
    "GateAuthDomain",
    "GateAccountDomain",
    "GateAccountCore",
    "GateMediaDomain",
    "GateMediaCore",
    "GateMomentsDomain",
    "GateCallDomain",
    "GateCallCore",
    "GateR18Domain",
    "GateR18Core",
)

# The six shared infra slices. None may depend on a domain target (principle 1:
# the shared layer is pure mechanism; it cannot reach up into business policy).
INFRA_SLICES = (
    "GateInfraRuntime",
    "GateInfraConfig",
    "GateInfraCache",
    "GateInfraClients",
    "GateInfraPersistence",
    "GateInfraSupport",
)

# The domains whose SQL currently still lives in the wide shared PostgresDao.
# This is the documented, ratcheted exception. Each MUST have a service-local
# persistence seam in its OWNING service dir, and the set may only SHRINK — a new
# domain reaching into the shared DAO is the drift this test blocks.
DOMAIN_PERSISTENCE_SEAMS = {
    "Moments": SERVER_CORE / "MomentsService" / "domain" / "services" / "moments" / "MomentsPersistence.hpp",
    "Media": SERVER_CORE / "MediaService" / "domain" / "services" / "media" / "MediaPersistence.hpp",
    "Call": SERVER_CORE / "CallService" / "core" / "support" / "CallPersistence.hpp",
    "Account": SERVER_CORE / "AccountShared" / "domain" / "services" / "account" / "AccountPersistence.hpp",
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def link_block(cmake: str, target: str) -> str:
    """Concatenated bodies of every target_link_libraries(<target> ...) call."""
    blocks = re.findall(
        rf"target_link_libraries\(\s*{re.escape(target)}\b(.*?)\)",
        cmake,
        re.DOTALL,
    )
    return "\n".join(blocks)


class MechanismNotPolicyTest(unittest.TestCase):
    """Principle 1: the shared layer shares stable mechanism, never business policy.

    Concretely: dependency direction is domain -> shared only. No shared infra
    slice may link a business-domain library, so business policy can never be
    pulled down into (or reached from) the shared base.
    """

    def test_no_infra_slice_depends_on_a_domain_target(self):
        cmake = read(INFRA_CMAKE)
        for slice_name in INFRA_SLICES:
            block = link_block(cmake, slice_name)
            for domain in DOMAIN_TARGETS:
                with self.subTest(slice=slice_name, domain=domain):
                    self.assertNotRegex(
                        block,
                        rf"\b{re.escape(domain)}\b",
                        f"{slice_name} links domain target {domain}: the shared infra "
                        f"layer must never depend on a business domain (principle 1). "
                        f"Dependency flows domain -> shared, never the reverse.",
                    )

    def test_pooled_sql_connection_is_domain_agnostic_mechanism(self):
        """The shared connection pool is the legitimately-shared MECHANISM. It
        must stay free of any business-domain table/column vocabulary so it can
        never quietly turn into a place to put domain SQL."""
        pool_header = read(GATE_SHARED / "core" / "persistence" / "PostgresDao.hpp")
        pool_match = re.search(r"class PostgresPool\b.*?\n};", pool_header, re.DOTALL)
        self.assertIsNotNone(pool_match, "PostgresPool (the shared pool mechanism) must exist.")
        pool_src = pool_match.group(0)
        domain_vocabulary = ("moment", "media_", "call_session", "friend", "group_", "r18")
        for token in domain_vocabulary:
            with self.subTest(token=token):
                self.assertNotIn(
                    token,
                    pool_src.lower(),
                    f"PostgresPool must stay a domain-agnostic mechanism; found business "
                    f"token '{token}'. Business SQL belongs in a domain-owned DAO.",
                )


class ServiceOwnedCodeTest(unittest.TestCase):
    """Principle 3: service-specific code lives in its owning service directory.

    GateShared keeps only neutral, cross-cutting modules (health + the runtime
    framework). No business-domain route module may be defined under GateShared.
    """

    def test_gateshared_modules_are_neutral_only(self):
        modules_dir = GATE_SHARED / "modules"
        business_markers = ("moments", "media", "call", "auth", "profile", "r18", "/ai/")
        offenders = []
        for cpp in modules_dir.rglob("*.cpp"):
            lowered = cpp.read_text(encoding="utf-8-sig").lower()
            name = cpp.name.lower()
            if any(m in name for m in business_markers):
                offenders.append(cpp.relative_to(REPO_ROOT).as_posix())
                continue
            # Health + module-probe are the only sanctioned neutral modules.
            if "health" in name or "moduleprobe" in name:
                continue
            # Any other module compiled here must not register a business route.
            if "registerroute" in lowered.replace(" ", "") and any(m in lowered for m in business_markers):
                offenders.append(cpp.relative_to(REPO_ROOT).as_posix())
        self.assertEqual(
            [],
            offenders,
            f"GateShared/modules must hold only neutral cross-cutting modules "
            f"(health). Business route modules belong in their owning service: {offenders}",
        )

    def test_each_shared_dao_domain_has_a_service_local_seam(self):
        """Every domain whose SQL still sits in the wide shared DAO must expose a
        persistence seam in ITS OWN service dir — the migration target that lets
        the SQL eventually follow. A domain in the shared DAO with no local seam
        is exactly the un-owned coupling this principle forbids."""
        for domain, seam in DOMAIN_PERSISTENCE_SEAMS.items():
            with self.subTest(domain=domain):
                self.assertTrue(
                    seam.is_file(),
                    f"{domain} SQL lives in the shared DAO but has no service-local "
                    f"persistence seam at {seam.relative_to(REPO_ROOT)}.",
                )


class SharedDaoRatchetTest(unittest.TestCase):
    """Principle 1+3 ratchet: the wide fused PostgresMgr/PostgresDao facade is a
    KNOWN, documented exception that may only SHRINK. It freezes the exact public
    method surface of the shared DAO facade, so relocating a domain's SQL out
    (fewer methods) passes, while bolting a NEW method on (a new business policy
    creeping into shared infra) trips the ratchet until it is either given a
    service-owned DAO or the baseline is consciously, reviewably updated.

    A frozen method-name set is used rather than domain-keyword scanning because
    keyword scanning is brittle (e.g. the SQL keyword ORDER BY reads as an
    'order' domain). Subset semantics give a precise ratchet: the current surface
    must be a subset of the baseline captured at the time this contract landed."""

    # Frozen baseline: the PostgresMgr facade surface at the time the governance
    # contract landed. GOAL: this set only shrinks as domain SQL relocates to
    # service-owned DAOs. Adding a name is a deliberate, reviewed act (it means a
    # new method — and likely new business policy — entered the shared DAO).
    FROZEN_FACADE_BASELINE = frozenset(
        {
            "AddMoment",
            "AddMomentComment",
            "AddMomentCommentLike",
            "AddMomentLike",
            "AttestAdultForR18",
            "CanViewMoment",
            "CheckEmail",
            "CheckPwd",
            "DeleteMoment",
            "DeleteMomentComment",
            "GetCallSession",
            "GetCallUserProfile",
            "GetMediaAssetByKey",
            "GetMomentById",
            "GetMomentCommentById",
            "GetMomentCommentLikes",
            "GetMomentComments",
            "GetMomentLikes",
            "GetMomentsFeed",
            "GetMomentsFeedCandidates",
            "GetR18AccessPolicy",
            "GetUserInfo",
            "GetUserPublicId",
            "GrantMediaAccess",
            "GrantMediaGroupAccess",
            "HasLikedMoment",
            "HasLikedMomentComment",
            "HasMediaAccess",
            "InsertMediaAsset",
            "IsFriend",
            "IssueRefreshToken",
            "RegUser",
            "RemoveMomentCommentLike",
            "RemoveMomentLike",
            "ResolveActiveRefreshTokenUserId",
            "RevokeAllRefreshTokensForUid",
            "RevokeRefreshToken",
            "RotateRefreshToken",
            "TestProcedure",
            "UpdatePwd",
            "UpdateUserProfile",
            "UpsertCallSession",
        }
    )

    FACADE_HEADER = GATE_SHARED / "core" / "persistence" / "PostgresMgr.hpp"

    # Lifecycle / connection-mechanism names that are NOT domain business
    # policy: the ctor/dtor (== class name), the singleton accessor, and the
    # readiness mechanism. Excluded so the ratchet tracks only domain methods
    # regardless of header formatting (e.g. a [[nodiscard]] prefix change).
    NON_DOMAIN_NAMES = frozenset({"PostgresMgr", "GetInstance", "Ready", "StartupError"})

    @classmethod
    def facade_method_names(cls, header_src: str) -> set[str]:
        """Domain-business method names declared on the PostgresMgr facade,
        excluding ctor/dtor, the singleton accessor and the readiness mechanism."""
        names = set()
        for match in re.finditer(
            r"^\s+(?:\[\[nodiscard\]\]\s*)?[A-Za-z_][\w:<>,&*\s]*?\b([A-Z]\w*)\s*\(", header_src, re.M
        ):
            names.add(match.group(1))
        return names - cls.NON_DOMAIN_NAMES

    def test_shared_dao_is_documented_as_a_migration_target(self):
        self.assertTrue(LAYERING_DOC.is_file(), "docs/gateshared-layering.md must exist.")
        doc = read(LAYERING_DOC)
        self.assertIn(
            "PostgresDao",
            doc,
            "The layering doc must name the wide shared DAO as a known, ratcheted exception.",
        )

    def test_shared_dao_facade_only_shrinks(self):
        """The current facade surface must be a SUBSET of the frozen baseline: no
        new method may be added to the shared DAO."""
        current = self.facade_method_names(read(self.FACADE_HEADER))
        added = current - self.FROZEN_FACADE_BASELINE
        self.assertEqual(
            set(),
            added,
            f"New method(s) {sorted(added)} added to the shared PostgresMgr/PostgresDao. "
            f"The shared DAO is a frozen, shrink-only migration target: new persistence "
            f"belongs in a service-owned DAO. If this is a deliberate account-domain "
            f"addition, update FROZEN_FACADE_BASELINE with a reviewed commit.",
        )


if __name__ == "__main__":
    unittest.main()
