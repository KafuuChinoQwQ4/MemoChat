from pathlib import Path

from tests.python.support.paths import repo_root

ROOT = repo_root()
SERVER_CORE = ROOT / "apps" / "server" / "core"
GATE_SHARED = SERVER_CORE / "GateShared"

LEGACY_ROUTE_DIRS = {
    "h3 legacy routes": GATE_SHARED / "transports" / "h3" / "legacy_routes",
}

EXPECTED_COMPAT_FILES = {
    "h3 legacy routes": {
        "GateHttp3ServiceRoutes.cpp",
        "GateHttp3ServiceRoutes.h",
        "_TREE.md",
    },
}


def test_gate_compatibility_inventory_documents_legacy_transport_boundaries() -> None:
    inventory = (SERVER_CORE / "docs" / "gate-compatibility-inventory.md").read_text(encoding="utf-8")

    assert "GateShared/transports/h3/legacy_routes/**" in inventory
    assert "Do not add new product routes here" in inventory
    assert "focused route modules" in inventory


def test_legacy_transport_route_directories_stay_inventory_only() -> None:
    for label, directory in LEGACY_ROUTE_DIRS.items():
        actual = {path.relative_to(directory).as_posix() for path in directory.rglob("*") if path.is_file()}
        assert actual == EXPECTED_COMPAT_FILES[label], (
            f"{label} changed. Add new product routes under GateShared/modules/**; "
            "update gate-compatibility-inventory.md only for deliberate compatibility work."
        )
