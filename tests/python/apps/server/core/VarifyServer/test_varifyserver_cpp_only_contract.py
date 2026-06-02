import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
VARIFY_SERVER_DIR = REPO_ROOT / "apps/server/core/VarifyServer"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


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
        source = read(REPO_ROOT / "README.md")

        self.assertIn("VarifyServer", source)
        self.assertIn("**核心服务**", source)
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
        for relative_path in (
            "tools/scripts/start_varify.bat",
            "tools/scripts/run_verifyserver.ps1",
        ):
            source = read(REPO_ROOT / relative_path)
            self.assertIn("VarifyServer", source)
            self.assertNotIn("node server.js", source)
            self.assertNotIn("npm install", source)
            self.assertNotIn("node_modules", source)

    def test_legacy_scripts_do_not_start_node_varifyserver(self):
        for relative_path in (
            "tools/scripts/start_and_monitor.ps1",
            "tools/scripts/start-windows-dev.ps1",
        ):
            source = read(REPO_ROOT / relative_path)
            self.assertIn("VarifyServer", source)
            self.assertNotIn("VarifyServer (Node.js)", source)
            self.assertNotIn('Start-Process -FilePath "node"', source)
            self.assertNotIn("server.js", source)
            self.assertNotIn("npm install", source)
            self.assertNotIn("node_modules", source)

    def test_legacy_verify_code_helpers_do_not_import_varifyserver_node_dependencies(self):
        for relative_path in (
            "tools/scripts/node_test.js",
            "tools/scripts/full_login_test.js",
            "tools/scripts/read_verify.js",
            "tools/scripts/poll_verify.js",
            "tools/scripts/get_and_read_code.ps1",
        ):
            source = read(REPO_ROOT / relative_path)
            self.assertNotIn("server/VarifyServer/node_modules", source)
            self.assertNotIn("VarifyServer/node_modules", source)
            self.assertNotIn("ioredis", source)
            self.assertNotIn("NODE_PATH", source)

    def test_linux_runtime_scripts_still_deploy_and_start_cpp_binary(self):
        deploy = read(REPO_ROOT / "tools/scripts/status/deploy_services.sh")
        start = read(REPO_ROOT / "tools/scripts/status/start-all-services.sh")

        self.assertIn('require_file "${BUILD_BIN}/VarifyServer"', deploy)
        self.assertIn('deploy_service "VarifyServer1" "VarifyServer" "VarifyServer" "VarifyServer/config.ini"', deploy)
        self.assertIn('deploy_service "VarifyServer2" "VarifyServer" "VarifyServer" "VarifyServer/varify2.ini"', deploy)
        self.assertIn('launch_svc "${RUNTIME_DIR}/VarifyServer1" "VarifyServer" "VarifyServer-1" "50051"', start)
        self.assertIn('launch_svc "${RUNTIME_DIR}/VarifyServer2" "VarifyServer" "VarifyServer-2" "48083"', start)

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
        source = read(REPO_ROOT / "tools/scripts/preflight.ps1")

        self.assertIn("server/VarifyServer/config.ini", source)
        self.assertIn('"VarifyServer.exe"', source)
        self.assertNotIn("server/VarifyServer/config.json", source)
        self.assertNotIn("server/VarifyServer/package.json", source)
        self.assertNotIn('"node"', source)
        self.assertNotIn('"npm"', source)


if __name__ == "__main__":
    unittest.main()
