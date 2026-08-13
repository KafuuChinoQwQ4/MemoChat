import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
PACKAGER = REPO_ROOT / "tools/scripts/release/package_backend_services.sh"


@unittest.skipUnless(shutil.which("gcc") and shutil.which("patchelf"), "requires gcc and patchelf")
class CppServiceBundleTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        self.build_bin = self.root / "build" / "bin"
        self.source_lib = self.root / "build" / "lib"
        self.build_bin.mkdir(parents=True)
        self.source_lib.mkdir(parents=True)

        library_source = self.root / "msquic.c"
        library_source.write_text("int memochat_msquic_probe(void) { return 7; }\n", encoding="utf-8")
        library = self.source_lib / "libmsquic.so.2"
        subprocess.run(
            [
                "gcc",
                "-shared",
                "-fPIC",
                "-Wl,-soname,libmsquic.so.2",
                "-o",
                str(library),
                str(library_source),
            ],
            check=True,
        )

        executable_source = self.root / "service.c"
        executable_source.write_text(
            "int memochat_msquic_probe(void);\nint main(void) { return memochat_msquic_probe() == 7 ? 0 : 1; }\n",
            encoding="utf-8",
        )
        subprocess.run(
            [
                "gcc",
                str(executable_source),
                f"-L{self.source_lib}",
                "-Wl,-rpath," + str(self.source_lib),
                "-Wl,--no-as-needed",
                "-l:libmsquic.so.2",
                "-o",
                str(self.build_bin / "LoginServer"),
            ],
            check=True,
        )

        (self.build_bin / "server.key").write_text("PRIVATE KEY sentinel\n", encoding="utf-8")
        (self.build_bin / "credentials.json").write_text('{"token":"sentinel"}\n', encoding="utf-8")

        self.vcpkg_installed = self.root / "vcpkg-installed"
        self.vcpkg_triplet = "x64-linux-memochat-release"
        package_share = self.vcpkg_installed / self.vcpkg_triplet / "share" / "zlib"
        package_share.mkdir(parents=True)
        package_share.joinpath("vcpkg.spdx.json").write_text(
            json.dumps(
                {
                    "spdxVersion": "SPDX-2.2",
                    "SPDXID": "SPDXRef-DOCUMENT",
                    "name": "zlib fixture",
                    "dataLicense": "CC0-1.0",
                    "documentNamespace": "https://example.invalid/zlib-fixture",
                    "creationInfo": {
                        "created": "1970-01-01T00:00:00Z",
                        "creators": ["Tool: vcpkg-test"],
                    },
                    "packages": [
                        {
                            "name": "zlib",
                            "SPDXID": "SPDXRef-Package-zlib",
                            "versionInfo": "1.3.1",
                            "downloadLocation": "NOASSERTION",
                            "filesAnalyzed": False,
                            "licenseConcluded": "Zlib",
                            "licenseDeclared": "Zlib",
                            "copyrightText": "NOASSERTION",
                        }
                    ],
                }
            )
            + "\n",
            encoding="utf-8",
        )
        status = self.vcpkg_installed / "vcpkg" / "status"
        status.parent.mkdir(parents=True)
        status.write_text(
            f"Package: zlib\nVersion: 1.3.1\nArchitecture: {self.vcpkg_triplet}\nStatus: install ok installed\n",
            encoding="utf-8",
        )

    def tearDown(self):
        self.temp_dir.cleanup()

    def run_packager(
        self,
        output: Path,
        *extra: str,
        environment: dict[str, str] | None = None,
        packager: Path = PACKAGER,
        include_library_dir: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        command = [
            "bash",
            str(packager),
            "--build-bin",
            str(self.build_bin),
            "--output",
            str(output),
            "--target",
            "LoginServer",
        ]
        if include_library_dir:
            command.extend(("--library-dir", str(self.source_lib)))
        command.extend(extra)
        env = os.environ.copy()
        env.update(environment or {})
        return subprocess.run(
            command,
            cwd=REPO_ROOT,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def test_creates_relocatable_allowlisted_service_bundle(self):
        output = self.root / "release"
        result = self.run_packager(output)
        self.assertEqual(0, result.returncode, result.stdout)

        service = output / "LoginServer"
        executable = service / "bin" / "LoginServer"
        library = service / "lib" / "libmsquic.so.2"
        self.assertTrue(executable.is_file())
        self.assertTrue(library.is_file())
        self.assertTrue((service / "MANIFEST.txt").is_file())
        self.assertTrue((service / "SHA256SUMS").is_file())
        legal_status = service.joinpath("legal/LEGAL-STATUS.txt").read_text(encoding="utf-8")
        self.assertIn("third_party_legal_corpus=complete", legal_status)
        self.assertIn("formal_distribution_ready=false", legal_status)
        manifest = service.joinpath("MANIFEST.txt").read_text(encoding="utf-8")
        self.assertIn("legal_inventory=complete", manifest)
        self.assertIn("third_party_legal_corpus=complete", manifest)
        self.assertIn("formal_distribution_ready=false", manifest)
        self.assertFalse(any(output.rglob("*.key")))
        self.assertFalse(any(output.rglob("credentials.json")))

        rpath = subprocess.check_output(["patchelf", "--print-rpath", str(executable)], text=True).strip()
        self.assertEqual("$ORIGIN/../lib", rpath)
        self.assertNotIn(str(self.source_lib), rpath)
        self.assertNotIn(str(self.source_lib).encode(), executable.read_bytes())

        env = os.environ.copy()
        env["LD_LIBRARY_PATH"] = str(service / "lib")
        run = subprocess.run([str(executable)], env=env, check=False)
        self.assertEqual(0, run.returncode)

    def test_packages_a_validated_vcpkg_installed_closure_sbom(self):
        output = self.root / "release-with-sbom"

        result = self.run_packager(
            output,
            "--vcpkg-installed-root",
            str(self.vcpkg_installed),
            "--vcpkg-triplet",
            self.vcpkg_triplet,
        )

        self.assertEqual(0, result.returncode, result.stdout)
        service = output / "LoginServer"
        sbom_path = service / "sbom/vcpkg-build-dependencies.spdx.json"
        self.assertTrue(sbom_path.is_file())
        sbom = json.loads(sbom_path.read_text(encoding="utf-8"))
        self.assertEqual("SPDX-2.3", sbom["spdxVersion"])
        self.assertEqual(
            "release_source_sha=unbound; coverage=installed-closure-overapproximation",
            sbom["documentComment"],
        )
        self.assertEqual(["zlib"], [package["name"] for package in sbom["packages"]])
        manifest = service.joinpath("MANIFEST.txt").read_text(encoding="utf-8")
        self.assertIn("vcpkg_sbom_coverage=installed-closure-overapproximation", manifest)
        self.assertRegex(manifest, r"(?m)^vcpkg_sbom_sha256=[0-9a-f]{64}$")
        self.assertIn("vcpkg_sbom_source_sha=unbound", manifest)
        self.assertRegex(manifest, r"(?m)^legal_status_sha256=[0-9a-f]{64}$")
        subprocess.run(
            ["sha256sum", "--check", "--strict", "SHA256SUMS"],
            cwd=service,
            check=True,
            capture_output=True,
            text=True,
        )

    def test_allows_verified_bundle_output_below_the_current_user_home(self):
        with tempfile.TemporaryDirectory(prefix="memochat-bundle-home-", dir=Path.home()) as home_temp:
            output = Path(home_temp) / "release"
            result = self.run_packager(output)

            self.assertEqual(0, result.returncode, result.stdout)
            self.assertTrue((output / "LoginServer/bin/LoginServer").is_file())

    def test_refuses_to_merge_into_existing_output(self):
        output = self.root / "release"
        output.mkdir()
        marker = output / "keep.txt"
        marker.write_text("user data\n", encoding="utf-8")

        result = self.run_packager(output)
        self.assertNotEqual(0, result.returncode)
        self.assertEqual("user data\n", marker.read_text(encoding="utf-8"))

    def test_refuses_unknown_service_target(self):
        output = self.root / "release"
        result = subprocess.run(
            [
                "bash",
                str(PACKAGER),
                "--build-bin",
                str(self.build_bin),
                "--output",
                str(output),
                "--target",
                "GateServer",
                "--library-dir",
                str(self.source_lib),
            ],
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertNotEqual(0, result.returncode)
        self.assertIn("unsupported service target", result.stdout.lower())

    def test_bundles_gcc_runtime_even_when_resolved_from_system_directories(self):
        script = PACKAGER.read_text(encoding="utf-8")
        for soname in ("libstdc++.so.6", "libgcc_s.so.1", "libatomic.so.1"):
            with self.subTest(soname=soname):
                self.assertIn(soname, script)
        self.assertIn("is_redistributable_compiler_runtime", script)
        self.assertIn("--shrink-rpath", script)
        self.assertIn("--allowed-rpath-prefixes", script)

    def test_requires_an_explicit_non_system_library_directory(self):
        output = self.root / "release"

        result = self.run_packager(output, include_library_dir=False)

        self.assertNotEqual(0, result.returncode)
        self.assertIn("--library-dir is required", result.stdout)

    def test_ignores_ambient_loader_injection_and_packages_the_allowlisted_library(self):
        ambient_lib = self.root / "ambient"
        ambient_lib.mkdir()
        malicious_source = self.root / "malicious-msquic.c"
        malicious_source.write_text("int memochat_msquic_probe(void) { return 99; }\n", encoding="utf-8")
        malicious_library = ambient_lib / "libmsquic.so.2"
        subprocess.run(
            [
                "gcc",
                "-shared",
                "-fPIC",
                "-Wl,-soname,libmsquic.so.2",
                "-o",
                str(malicious_library),
                str(malicious_source),
            ],
            check=True,
        )
        output = self.root / "release"

        result = self.run_packager(
            output,
            environment={
                "LD_LIBRARY_PATH": str(ambient_lib),
                "LD_PRELOAD": str(malicious_library),
            },
        )

        self.assertEqual(0, result.returncode, result.stdout)
        service = output / "LoginServer"
        run_env = os.environ.copy()
        run_env["LD_LIBRARY_PATH"] = str(service / "lib")
        packaged_run = subprocess.run(
            [str(service / "bin/LoginServer")],
            env=run_env,
            check=False,
        )
        self.assertEqual(0, packaged_run.returncode)

    def test_rejects_non_system_dependency_outside_the_allowlisted_roots(self):
        empty_allowed_root = self.root / "allowed"
        empty_allowed_root.mkdir()
        output = self.root / "release"

        result = self.run_packager(
            output,
            "--library-dir",
            str(empty_allowed_root),
            include_library_dir=False,
        )

        self.assertNotEqual(0, result.returncode)
        self.assertIn("outside explicit --library-dir roots", result.stdout)

    def test_includes_verified_repository_legal_inventory(self):
        fixture_repo = self.root / "fixture-repo"
        fixture_script = fixture_repo / "tools/scripts/release/package_backend_services.sh"
        fixture_verifier = fixture_repo / "tools/scripts/release/verify_release_legal.sh"
        fixture_snapshot = fixture_script.parent / "compute_release_source_snapshot.py"
        fixture_script.parent.mkdir(parents=True)
        shutil.copy2(PACKAGER, fixture_script)
        shutil.copy2(REPO_ROOT / "tools/scripts/release/verify_release_legal.sh", fixture_verifier)
        shutil.copy2(
            REPO_ROOT / "tools/scripts/release/compute_release_source_snapshot.py",
            fixture_snapshot,
        )
        fixture_script.chmod(0o755)
        fixture_verifier.chmod(0o755)
        fixture_snapshot.chmod(0o755)
        shutil.copy2(REPO_ROOT / "LICENSE", fixture_repo / "LICENSE")
        shutil.copy2(REPO_ROOT / "THIRD_PARTY_NOTICES.md", fixture_repo / "THIRD_PARTY_NOTICES.md")
        output = self.root / "release-with-legal"

        result = self.run_packager(output, packager=fixture_script)

        self.assertEqual(0, result.returncode, result.stdout)
        service = output / "LoginServer"
        self.assertEqual(
            fixture_repo.joinpath("LICENSE").read_bytes(),
            service.joinpath("legal/LICENSE").read_bytes(),
        )
        self.assertEqual(
            fixture_repo.joinpath("THIRD_PARTY_NOTICES.md").read_bytes(),
            service.joinpath("legal/THIRD_PARTY_NOTICES.md").read_bytes(),
        )
        self.assertTrue(service.joinpath("legal/LEGAL-STATUS.txt").is_file())
        checksums = service.joinpath("SHA256SUMS").read_text(encoding="utf-8")
        self.assertIn("legal/LICENSE", checksums)
        self.assertIn("legal/THIRD_PARTY_NOTICES.md", checksums)
        self.assertIn("legal/LEGAL-STATUS.txt", checksums)

        dockerfile = REPO_ROOT.joinpath("infra/deploy/images/services/cpp-service.Dockerfile").read_text(
            encoding="utf-8"
        )
        self.assertIn("/legal/", dockerfile)


if __name__ == "__main__":
    unittest.main()
