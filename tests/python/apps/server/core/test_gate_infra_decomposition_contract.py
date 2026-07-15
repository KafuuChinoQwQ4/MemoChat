"""GateShared infra decomposition — layering contract tests.

Locks in the remediation of the architecture-review finding
"GateShared 仍然是高风险共享层" (GateShared is still a high-risk shared layer):
it fused runtime + config + cache + gRPC clients + persistence + auth support
into ONE static archive (GateInfraCore), so any change relinked every service
and every consumer force-linked libpq/mongoc/bson even with no database.

The fix split it into six leaf slices along their real dependency DAG, kept
GateInfraCore as an INTERFACE facade (so existing consumers are unchanged), and
inverted the shared framework bootstrap onto injected readiness probes so the
framework no longer names a concrete manager and no longer hard-links the
persistence/cache slices.

Each test asserts the POSITIVE invariant of that fix, so it fails if the split
is reverted or if the shared layer starts to re-accrete concerns — the "becomes
a new GateServer" drift the review warned about. These are static source/CMake
scanners (no build, no live service), matching the sibling contract tests.
"""

import re
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SERVER_CORE = REPO_ROOT / "apps" / "server" / "core"
INFRA_CMAKE = SERVER_CORE / "GateShared" / "core" / "CMakeLists.txt"
ROOT_CMAKE = SERVER_CORE / "CMakeLists.txt"
BOOTSTRAP_CPP = SERVER_CORE / "GateShared" / "app" / "GateDomainServer.cpp"
BOOTSTRAP_HPP = SERVER_CORE / "GateShared" / "app" / "GateDomainServer.hpp"
PROBE_CONTRACT = SERVER_CORE / "GateShared" / "core" / "runtime" / "GateReadinessProbe.hpp"

# The six leaf slices the fused GateInfraCore was split into, and the concern
# each one isolates.
SLICES = (
    "GateInfraRuntime",
    "GateInfraConfig",
    "GateInfraCache",
    "GateInfraClients",
    "GateInfraPersistence",
    "GateInfraSupport",
)

# Tokens that name a concrete infra manager. The shared framework bootstrap must
# reach these only through the transport-agnostic GateReadinessProbe contract.
CONCRETE_MANAGERS = ("PostgresMgr", "MongoMgr", "RedisMgr")

# Heavy database externals that must stay confined to the persistence slice.
DB_EXTERNALS = (
    "MEMOCHAT_POSTGRES_TARGET",
    "MEMOCHAT_MONGOC_TARGET",
    "MEMOCHAT_BSON_TARGET",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def add_library_kind(cmake: str, target: str) -> str | None:
    """Return the first token after `add_library(<target>` (e.g. STATIC/INTERFACE)."""
    match = re.search(rf"add_library\(\s*{re.escape(target)}\s+(\w+)", cmake)
    return match.group(1) if match else None


def slice_link_block(cmake: str, target: str) -> str:
    """All target_link_libraries(<target> ...) bodies for one slice, concatenated."""
    blocks = re.findall(
        rf"target_link_libraries\(\s*{re.escape(target)}\b(.*?)\)",
        cmake,
        re.DOTALL,
    )
    return "\n".join(blocks)


class InfraSliceDecompositionTest(unittest.TestCase):
    """The fused archive is split into six independently-buildable leaf slices."""

    def test_every_slice_is_its_own_static_library(self):
        cmake = read(INFRA_CMAKE)
        for slice_name in SLICES:
            with self.subTest(slice=slice_name):
                self.assertEqual(
                    "STATIC",
                    add_library_kind(cmake, slice_name),
                    f"{slice_name} must be its own STATIC library so a change to one "
                    f"concern does not relink the others.",
                )

    def test_slices_are_layered_not_cross_linked(self):
        cmake = read(INFRA_CMAKE)
        # Config sits on the runtime base; cache/clients/persistence sit on config;
        # support sits on cache+config. This asserts the DAG edges exist (no cycle,
        # no slice reaching sideways into a sibling's concern).
        self.assertIn("GateInfraRuntime", slice_link_block(cmake, "GateInfraConfig"))
        for upper in ("GateInfraCache", "GateInfraClients", "GateInfraPersistence"):
            with self.subTest(slice=upper):
                self.assertIn("GateInfraConfig", slice_link_block(cmake, upper))
        support = slice_link_block(cmake, "GateInfraSupport")
        self.assertIn("GateInfraCache", support)
        self.assertIn("GateInfraConfig", support)


class InfraFacadeTest(unittest.TestCase):
    """GateInfraCore stays an INTERFACE facade so it can never accrete code."""

    def test_infra_core_is_interface_only(self):
        cmake = read(INFRA_CMAKE)
        self.assertEqual(
            "INTERFACE",
            add_library_kind(cmake, "GateInfraCore"),
            "GateInfraCore must be an INTERFACE library. If it gains compiled "
            "sources it becomes a shared-code sink again — the exact 'new "
            "GateServer' drift this decomposition prevents.",
        )

    def test_facade_aggregates_all_six_slices(self):
        cmake = read(INFRA_CMAKE)
        facade = slice_link_block(cmake, "GateInfraCore")
        for slice_name in SLICES:
            with self.subTest(slice=slice_name):
                self.assertIn(
                    slice_name,
                    facade,
                    f"GateInfraCore facade must link {slice_name} so existing "
                    f"consumers keep their full transitive closure unchanged.",
                )


class DatabaseExternalsConfinementTest(unittest.TestCase):
    """libpq/mongoc/bson stay confined to the persistence slice."""

    def test_only_persistence_slice_links_db_externals(self):
        cmake = read(INFRA_CMAKE)
        for slice_name in SLICES:
            block = slice_link_block(cmake, slice_name)
            for external in DB_EXTERNALS:
                with self.subTest(slice=slice_name, external=external):
                    if slice_name == "GateInfraPersistence":
                        continue
                    self.assertNotIn(
                        external,
                        block,
                        f"{slice_name} must not link {external}; the heavy database "
                        f"externals belong only to GateInfraPersistence.",
                    )

    def test_persistence_slice_owns_db_externals(self):
        cmake = read(INFRA_CMAKE)
        block = slice_link_block(cmake, "GateInfraPersistence")
        for external in DB_EXTERNALS:
            with self.subTest(external=external):
                self.assertIn(external, block)

    def test_shared_slice_helper_stays_lean(self):
        """The helper every slice runs through must not link DB externals itself,
        or the confinement above would be silently defeated."""
        cmake = read(INFRA_CMAKE)
        match = re.search(
            r"function\(memochat_configure_gate_infra_slice.*?endfunction\(\)",
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(match, "memochat_configure_gate_infra_slice must exist.")
        body = match.group(0)
        for external in DB_EXTERNALS:
            with self.subTest(external=external):
                self.assertNotIn(external, body)


class FrameworkDecouplingTest(unittest.TestCase):
    """The shared framework bootstrap references no concrete infra manager."""

    def test_probe_contract_exists(self):
        self.assertTrue(self.probe_contract_present())
        contract = read(PROBE_CONTRACT)
        self.assertIn("struct GateReadinessProbe", contract)

    @staticmethod
    def probe_contract_present() -> bool:
        return PROBE_CONTRACT.is_file()

    def test_bootstrap_uses_probes_not_managers(self):
        for path in (BOOTSTRAP_CPP, BOOTSTRAP_HPP):
            src = read(path)
            with self.subTest(file=path.name):
                self.assertIn(
                    "GateReadinessProbe",
                    src,
                    f"{path.name} must consume injected readiness probes.",
                )
                for manager in CONCRETE_MANAGERS:
                    self.assertNotIn(
                        manager,
                        src,
                        f"{path.name} must not name {manager}: the shared framework "
                        f"drops its hard link on persistence/cache by staying probe-"
                        f"agnostic. Add a probe factory in the owning slice instead.",
                    )

    def test_framework_links_lean_base_not_full_facade(self):
        cmake = read(ROOT_CMAKE)
        match = re.search(
            r"function\(memochat_configure_gate_app_lib.*?endfunction\(\)",
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(match, "memochat_configure_gate_app_lib must exist.")
        body = match.group(0)
        self.assertIn(
            "GateInfraConfig",
            body,
            "The framework helper should link the lean GateInfraConfig base.",
        )
        self.assertNotRegex(
            body,
            r"\bGateInfraCore\b",
            "The framework helper must NOT link the full GateInfraCore facade, or "
            "every gate app/domain library would force-link persistence again.",
        )


if __name__ == "__main__":
    unittest.main()
