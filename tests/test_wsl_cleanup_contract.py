import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
CLEANUP_SCRIPT = REPO_ROOT / "tools/scripts/status/cleanup-wsl-stale.ps1"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class WslCleanupContractTests(unittest.TestCase):
    def test_cleanup_script_exists_and_supports_safe_parameters(self):
        source = read(CLEANUP_SCRIPT)

        self.assertIn("SupportsShouldProcess", source)
        for token in (
            "$Distro",
            "$ProjectPath",
            "$MaxAgeMinutes",
            "$Stop",
            "$IncludeMcp",
            "MEMOCHAT_WSL_DISTRO",
            "MEMOCHAT_WSL_PROJECT_ROOT",
            "/root/code/MemoChat-Qml-Drogon-linux",
        ):
            self.assertIn(token, source)

    def test_cleanup_script_has_no_broad_wsl_termination(self):
        source = read(CLEANUP_SCRIPT).lower()

        forbidden = (
            "wsl --terminate",
            "wsl.exe --terminate",
            "wsl --shutdown",
            "wsl.exe --shutdown",
            "stop-process -name wsl",
            "taskkill /im wsl.exe",
            "taskkill.exe /im wsl.exe",
        )
        for token in forbidden:
            self.assertNotIn(token, source)

    def test_stop_path_requires_explicit_stop_and_whatif_gate(self):
        source = read(CLEANUP_SCRIPT)

        self.assertIsNotNone(
            re.search(r"if\s*\(\s*-not\s+\$Stop\s*\).*?return", source, flags=re.S),
            msg="-Stop must be required before any Stop-Process path",
        )
        self.assertIn("$PSCmdlet.ShouldProcess", source)
        self.assertRegex(
            source,
            r"Stop-Process\s+-Id\s+\$candidate\.Pid",
            msg="cleanup must stop only selected process ids, not names",
        )
        self.assertIn("WillStop", source)

    def test_selection_is_limited_to_wsl_distro_project_and_age(self):
        source = read(CLEANUP_SCRIPT)

        self.assertIn("Get-CimInstance Win32_Process -Filter \"name='wsl.exe'\"", source)
        self.assertIn("$_.Name -ieq 'wsl.exe'", source)
        self.assertIn("Test-DistroMatch", source)
        self.assertIn("Test-ProjectMatch", source)
        self.assertIn("$ageMinutes -ge $MaxAgeMinutes", source)
        self.assertIn("'candidate'", source)

    def test_mcp_and_infrastructure_wrappers_are_protected_by_default(self):
        source = read(CLEANUP_SCRIPT).lower()

        for token in (
            "mcp",
            "context7",
            "filesystem",
            "postgres",
            "redis",
            "neo4j",
            "qdrant",
            "redpanda",
            "mongo",
            "rabbitmq",
            "minio",
            "prometheus",
            "grafana",
        ):
            self.assertIn(token, source)

        self.assertIn("protected-helper", source)
        self.assertRegex(source, r"isprotected.*-and\s+-not\s+\$includemcp")

    def test_transient_command_candidates_are_explicit(self):
        source = read(CLEANUP_SCRIPT)

        for token in (
            "docker run --rm",
            "/usr/bin/git -C",
            "git diff --check",
            "date -Is",
            "/bin/echo",
            "/bin/true",
            "python -m unittest",
            "python -m compileall",
            "cmake --build",
            "ninja -C",
            "chmod +x",
            "sed -n",
        ):
            self.assertIn(token, source)


if __name__ == "__main__":
    unittest.main()
