import json
import subprocess
import tempfile
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
SCRIPT = REPO_ROOT / "tools" / "scripts" / "dev" / "webtransport_provider_preflight.mjs"


class WebTransportProviderPreflightContractTests(unittest.TestCase):
    def run_preflight(self, *extra_args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["node", str(SCRIPT), *extra_args],
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

    def test_script_is_valid_node_syntax(self):
        result = self.run_preflight("--help")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("webtransport_provider_preflight.mjs", result.stdout)

    def test_missing_dependency_roots_are_deferred_without_strict_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            result = self.run_preflight(
                "--json",
                "--repo-root",
                str(REPO_ROOT),
                "--vcpkg-root",
                str(tmp_path / "missing-vcpkg"),
                "--installed-dir",
                str(tmp_path / "missing-installed"),
                "--build-dir",
                str(tmp_path / "missing-build"),
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual(payload["status"], "DEFERRED")
        self.assertFalse(payload["providerEnabled"])
        self.assertEqual(payload["selectedLibwebsocketsPortSource"], "repo-overlay")
        self.assertNotIn("vcpkg-libwebsockets-port-present", payload["failures"])
        self.assertNotIn("vcpkg-libwebsockets-port-h3-webtransport", payload["failures"])
        self.assertIn("installed-libwebsockets-webtransport-header", payload["failures"])

    def test_strict_mode_fails_when_provider_prerequisites_are_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            result = self.run_preflight(
                "--json",
                "--strict",
                "--repo-root",
                str(REPO_ROOT),
                "--vcpkg-root",
                str(tmp_path / "missing-vcpkg"),
                "--installed-dir",
                str(tmp_path / "missing-installed"),
                "--build-dir",
                str(tmp_path / "missing-build"),
            )

        self.assertNotEqual(result.returncode, 0)
        payload = json.loads(result.stdout)
        self.assertEqual(payload["status"], "FAIL")
        self.assertTrue(payload["strict"])

    def test_output_contains_provider_specific_check_ids(self):
        result = self.run_preflight("--json")

        self.assertIn(result.returncode, {0, 1}, result.stderr)
        payload = json.loads(result.stdout)
        check_ids = {check["id"] for check in payload["checks"]}

        expected_ids = {
            "chatserver-cmake-provider-gate",
            "chatserver-transport-conditional-source",
            "provider-interface-boundary",
            "libwebsockets-provider-source",
            "unavailable-provider-default",
            "repo-libwebsockets-overlay-configured",
            "manifest-webtransport-provider-feature",
            "manifest-webtransport-provider-nondefault",
            "vcpkg-libwebsockets-port-present",
            "vcpkg-libwebsockets-port-h3-webtransport",
            "vcpkg-libwebsockets-h3-webtransport-patch",
            "installed-libwebsockets-core-header",
            "installed-libwebsockets-webtransport-header",
            "installed-libwebsockets-cmake-config",
            "cmake-cache-provider-option",
            "cmake-cache-webtransport-provider-feature",
        }
        self.assertTrue(expected_ids.issubset(check_ids))

        cmake_gate = next(check for check in payload["checks"] if check["id"] == "chatserver-cmake-provider-gate")
        self.assertIn("LWS_WITH_HTTP3", cmake_gate["evidence"]["tokens"])
        self.assertIn("LWS_ROLE_WT", cmake_gate["evidence"]["tokens"])
        self.assertIn("lws_wt_create_stream", cmake_gate["evidence"]["tokens"])

        h3_patch = next(
            check for check in payload["checks"] if check["id"] == "vcpkg-libwebsockets-h3-webtransport-patch"
        )
        self.assertIn("HTTP_STATUS_OK", h3_patch["evidence"]["tokens"])
        self.assertIn("LWS_WT_STREAM_TYPE_BIDI", h3_patch["evidence"]["tokens"])
        self.assertIn("wsi_child->role_ops->rx_cb", h3_patch["evidence"]["tokens"])
        self.assertIn("lws_get_quic_network_wsi", h3_patch["evidence"]["tokens"])
        self.assertIn("rops_callback_on_writable_wt", h3_patch["evidence"]["tokens"])
        self.assertIn(".callback_on_writable  = rops_callback_on_writable_wt", h3_patch["evidence"]["tokens"])
        self.assertIn("/* LWS_ROPS_handle_POLLIN */\t\t\t0x01", h3_patch["evidence"]["tokens"])
        self.assertIn("/* LWS_ROPS_tx_credit */\t\t\t0x40", h3_patch["evidence"]["tokens"])
        self.assertIn("/* LWS_ROPS_encapsulation_parent */\t\t0x20", h3_patch["evidence"]["tokens"])
        self.assertIn("/* LWS_ROPS_close_kill_connection */\t\t0x03", h3_patch["evidence"]["tokens"])
        self.assertIn("lws_wsi_mux_mark_parents_needing_writeable(wsi);", h3_patch["evidence"]["tokens"])
        self.assertIn("return lws_callback_on_writable(nwsi);", h3_patch["evidence"]["tokens"])

    def test_repo_overlay_port_is_selected_before_builtin_port(self):
        result = self.run_preflight("--json")

        self.assertIn(result.returncode, {0, 1}, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual(payload["selectedLibwebsocketsPortSource"], "repo-overlay")
        self.assertTrue(payload["selectedLibwebsocketsPortDir"].endswith("infra/ports/libwebsockets"))

        h3_check = next(
            check for check in payload["checks"] if check["id"] == "vcpkg-libwebsockets-port-h3-webtransport"
        )
        self.assertTrue(h3_check["ok"])
        self.assertEqual(h3_check["evidence"]["source"], "repo-overlay")

    def test_manifest_keeps_webtransport_provider_non_default(self):
        result = self.run_preflight("--json")

        self.assertIn(result.returncode, {0, 1}, result.stderr)
        payload = json.loads(result.stdout)
        checks = {check["id"]: check for check in payload["checks"]}

        self.assertTrue(checks["repo-libwebsockets-overlay-configured"]["ok"])
        self.assertTrue(checks["manifest-webtransport-provider-feature"]["ok"])
        self.assertIn("libwebsockets", checks["manifest-webtransport-provider-feature"]["evidence"]["dependencies"])
        self.assertTrue(checks["manifest-webtransport-provider-nondefault"]["ok"])
        self.assertNotIn(
            "webtransport-provider", checks["manifest-webtransport-provider-nondefault"]["evidence"]["defaultFeatures"]
        )


if __name__ == "__main__":
    unittest.main()
