import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
VARIFY_SERVER_DIR = REPO_ROOT / "apps/server/core/VarifyServer"
TOOLS_SCRIPTS_DIR = REPO_ROOT / "tools/scripts"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def find_script(name: str) -> Path:
    # tools/scripts was reorganized into category subdirectories; locate a script by name
    # anywhere under tools/scripts/ so the contract is robust to relocation (the contract is
    # "this script exists somewhere under tools/scripts and is C++-only", not its exact path).
    hits = sorted(TOOLS_SCRIPTS_DIR.rglob(name))
    assert len(hits) == 1, f"expected exactly one tools/scripts/**/{name}, found {hits}"
    return hits[0]


class VarifyServerCppOnlyContractTests(unittest.TestCase):
    def test_node_varifyserver_runtime_files_are_absent(self):
        node_runtime_files = (
            "server.js",
            "config.js",
            "redis.js",
            "rabbitmq.js",
            "email.js",
            "logger.js",
            "telemetry.js",
            "proto.js",
            "const.js",
            "package.json",
            "package-lock.json",
            "Dockerfile",
            "config.json",
        )

        existing = [name for name in node_runtime_files if (VARIFY_SERVER_DIR / name).exists()]

        self.assertEqual(existing, [], "VarifyServer must not keep Node.js runtime files")

    def test_readme_describes_varifyserver_as_cpp_service_not_nodejs(self):
        # README was trimmed to a minimal project intro and no longer maintains a service list,
        # so this is a negative-only regression guard: the README must never (re)introduce a
        # Node.js description of VarifyServer (it is a C++ service after the Node→C++ migration).
        source = read(REPO_ROOT / "README.md")

        self.assertNotIn("VarifyServer 使用 Node.js", source)
        self.assertNotIn("@grpc/grpc-js", source)
        self.assertNotIn("@grpc/proto-loader", source)
        self.assertNotIn("nodemailer", source)

    def test_windows_start_script_launches_cpp_varifyserver(self):
        source = read(REPO_ROOT / "tools/scripts/status/start-all-services.bat")

        self.assertIn("VarifyServer.exe", source)
        self.assertIn("VarifyServer1", source)
        self.assertIn("VarifyServer2", source)
        self.assertIn('"50051"', source)
        self.assertIn('"48083"', source)
        self.assertNotIn("node server.js", source)
        self.assertNotIn("npm install", source)
        self.assertNotIn("node_modules", source)

    def test_windows_stop_script_treats_varifyserver_as_cpp_backend(self):
        source = read(REPO_ROOT / "tools/scripts/status/stop-all-services.bat")

        self.assertIn("VarifyServer.exe", source)
        self.assertIn("50051,48083,8083,8087", source)
        self.assertNotIn("VarifyServer (Node.js)", source)
        self.assertNotIn("VarifyServer Node", source)

    def test_legacy_varify_launcher_does_not_start_node(self):
        for name in (
            "start_varify.bat",
            "run_verifyserver.ps1",
        ):
            source = read(find_script(name))
            self.assertIn("VarifyServer", source)
            self.assertNotIn("node server.js", source)
            self.assertNotIn("npm install", source)
            self.assertNotIn("node_modules", source)

    def test_legacy_scripts_do_not_start_node_varifyserver(self):
        for name in (
            "start_and_monitor.ps1",
            "start-windows-dev.ps1",
        ):
            source = read(find_script(name))
            self.assertIn("VarifyServer", source)
            self.assertNotIn("VarifyServer (Node.js)", source)
            self.assertNotIn('Start-Process -FilePath "node"', source)
            self.assertNotIn("server.js", source)
            self.assertNotIn("npm install", source)
            self.assertNotIn("node_modules", source)

    def test_legacy_verify_code_helpers_do_not_import_varifyserver_node_dependencies(self):
        for name in (
            "node_test.js",
            "full_login_test.js",
            "read_verify.js",
            "poll_verify.js",
            "get_and_read_code.ps1",
        ):
            source = read(find_script(name))
            self.assertNotIn("server/VarifyServer/node_modules", source)
            self.assertNotIn("VarifyServer/node_modules", source)
            self.assertNotIn("ioredis", source)
            self.assertNotIn("NODE_PATH", source)

    def test_linux_runtime_scripts_still_deploy_and_start_cpp_binary(self):
        # deploy/start were refactored to be data-driven: they source runtime_topology.sh and
        # iterate MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY instead of hardcoding each service. Verify the
        # VarifyServer C++ deploy/start contract lives in that shared topology table and that both
        # scripts consume it generically (require_file/deploy_service/launch_svc over the rows).
        topology = read(REPO_ROOT / "tools/scripts/status/runtime_topology.sh")
        deploy = read(REPO_ROOT / "tools/scripts/status/deploy_services.sh")
        start = read(REPO_ROOT / "tools/scripts/status/start-all-services.sh")

        # VarifyServer is a C++ binary (executable == source_executable == "VarifyServer"), two
        # instances, with their config files and TCP wait ports — encoded as topology rows.
        self.assertIn(
            "varify|VarifyServer1|VarifyServer|VarifyServer|VarifyServer/config.ini|VarifyServer-1|50051",
            topology,
        )
        self.assertIn(
            "varify|VarifyServer2|VarifyServer|VarifyServer|VarifyServer/varify2.ini|VarifyServer-2|48083",
            topology,
        )

        # Both scripts source the shared topology and iterate it (no per-service hardcoding).
        for source in (deploy, start):
            self.assertIn("runtime_topology.sh", source)
            self.assertIn('for row in "${MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY[@]}"; do', source)
        # deploy requires the built C++ binary and deploys each row's runtime dir + config.
        self.assertIn('require_file "${BUILD_BIN}/${source_exe}"', deploy)
        self.assertIn('deploy_service "$name" "$exe" "$source_exe" "$config"', deploy)
        # start launches each row's runtime dir, waiting on its TCP port.
        self.assertIn('launch_svc "${RUNTIME_DIR}/${name}" "$exe" "$display_name" "$tcp_wait_port"', start)

    def test_container_and_kubernetes_contracts_use_cpp_varifyserver(self):
        self.assertFalse(
            (REPO_ROOT / "infra/deploy/images/services/varifyserver.Dockerfile").exists(),
            "VarifyServer must use the shared C++ service Dockerfile",
        )

        for relative_path in (
            "infra/deploy/README.md",
            "infra/deploy/kubernetes/charts/memochat/README.md",
        ):
            source = read(REPO_ROOT / relative_path)
            self.assertIn("--build-arg TARGET=VarifyServer", source)
            self.assertIn("cpp-service.Dockerfile", source)
            self.assertNotIn("varifyserver.Dockerfile", source)
            self.assertNotIn("Node image for `VarifyServer`", source)

        images_readme = read(REPO_ROOT / "infra/deploy/images/README.md")
        self.assertIn("cpp-service.Dockerfile", images_readme)
        self.assertIn("VarifyServer", images_readme)
        self.assertNotIn("varifyserver.Dockerfile", images_readme)
        self.assertNotIn("Node image for `VarifyServer`", images_readme)

        varify_chart = read(REPO_ROOT / "infra/deploy/kubernetes/charts/memochat/templates/prod/varify.yaml")
        service_config = read(
            REPO_ROOT / "infra/deploy/kubernetes/charts/memochat/templates/bootstrap/configmap-services.yaml"
        )
        self.assertIn("mountPath: /app/config.ini", varify_chart)
        self.assertIn("subPath: varify.ini", varify_chart)
        self.assertIn("MEMOCHAT_EMAIL_SMTPPASS", varify_chart)
        self.assertNotIn("MEMOCHAT_EMAIL_PASS", varify_chart)
        self.assertIn("varify.ini: |", service_config)
        self.assertIn("[VarifyServer]", service_config)
        self.assertNotIn("config.json", varify_chart)
        self.assertNotIn("varify.config.json", service_config)

    def test_preflight_checks_cpp_varifyserver_inputs(self):
        source = read(find_script("preflight.ps1"))

        self.assertIn("server/VarifyServer/config.ini", source)
        self.assertIn('"VarifyServer.exe"', source)
        self.assertNotIn("server/VarifyServer/config.json", source)
        self.assertNotIn("server/VarifyServer/package.json", source)
        self.assertNotIn('"node"', source)
        self.assertNotIn('"npm"', source)


if __name__ == "__main__":
    unittest.main()
