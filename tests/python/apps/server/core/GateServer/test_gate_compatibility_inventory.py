from pathlib import Path

from tests.python.support.paths import repo_root

ROOT = repo_root()
SERVER_CORE = ROOT / "apps" / "server" / "core"
GATE_SHARED = SERVER_CORE / "GateShared"

LEGACY_ROUTE_DIRS = {
    "h1 legacy standalone": GATE_SHARED / "transports" / "h1" / "legacy_standalone",
    "h2 standalone handlers": GATE_SHARED / "transports" / "h2" / "handlers",
    "h2 standalone routes": GATE_SHARED / "transports" / "h2" / "routes",
    "h3 legacy routes": GATE_SHARED / "transports" / "h3" / "legacy_routes",
}

EXPECTED_COMPAT_FILES = {
    "h1 legacy standalone": {
        "CMakeLists.txt",
        "H1AuthRoutes.cpp",
        "H1AuthRoutes.h",
        "H1Connection.cpp",
        "H1Connection.h",
        "H1Globals.cpp",
        "H1Globals.h",
        "H1JsonSupport.cpp",
        "H1JsonSupport.h",
        "H1Listener.cpp",
        "H1Listener.h",
        "H1LogicSystem.cpp",
        "H1LogicSystem.h",
        "H1MediaService.cpp",
        "H1MediaService.h",
        "H1MomentsRoutes.cpp",
        "H1MomentsRoutes.h",
        "H1R18Routes.cpp",
        "H1R18Routes.h",
        "H1WorkerPool.cpp",
        "H1WorkerPool.h",
        "README.md",
        "config.ini",
        "main.cpp",
    },
    "h2 standalone handlers": {
        "Http2AuthHandlers.cpp",
        "Http2AuthHandlers.h",
        "Http2CallHandlers.cpp",
        "Http2CallHandlers.h",
        "Http2Handlers.cpp",
        "Http2Handlers.h",
        "Http2MediaHandlers.cpp",
        "Http2MediaHandlers.h",
        "Http2ProfileHandlers.cpp",
        "Http2ProfileHandlers.h",
    },
    "h2 standalone routes": {
        "Http2Routes.cpp",
        "Http2Routes.h",
    },
    "h3 legacy routes": {
        "GateHttp3ServiceRoutes.cpp",
        "GateHttp3ServiceRoutes.h",
    },
}


def test_gate_compatibility_inventory_documents_legacy_transport_boundaries() -> None:
    inventory = (SERVER_CORE / "docs" / "gate-compatibility-inventory.md").read_text(encoding="utf-8")

    assert "GateShared/transports/h1/legacy_standalone/**" in inventory
    assert "GateShared/transports/h2/**" in inventory
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
