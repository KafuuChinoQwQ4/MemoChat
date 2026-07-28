import json
import subprocess
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
PRESETS = REPO_ROOT / "CMakePresets.json"
RELEASE_TRIPLET = REPO_ROOT / "cmake/vcpkg-triplets/x64-linux-memochat-release.cmake"
MSQUIC_PORTFILE = REPO_ROOT / "cmake/vcpkg-overlay-ports/msquic/portfile.cmake"
OPENSSL_PORT = REPO_ROOT / "cmake/vcpkg-overlay-ports/openssl"
LIBPQ_PORT = REPO_ROOT / "cmake/vcpkg-overlay-ports/libpq"
MONGO_C_DRIVER_PORT = REPO_ROOT / "cmake/vcpkg-overlay-ports/mongo-c-driver"
NGHTTP2_PORTFILE = REPO_ROOT / "infra/ports/nghttp2/portfile.cmake"
NGHTTP2_MANIFEST = REPO_ROOT / "infra/ports/nghttp2/vcpkg.json"
SERVICE_IMAGE_README = REPO_ROOT / "infra/deploy/images/README.md"
LEGAL_APPROVAL_PACKAGERS = (
    (
        REPO_ROOT / "tools/scripts/release/package_linux_client.sh",
        "approval_public_key",
        "approval_signature",
        "legal_args",
    ),
    (
        REPO_ROOT / "tools/scripts/release/package_backend_services.sh",
        "APPROVAL_PUBLIC_KEY",
        "APPROVAL_SIGNATURE",
        "LEGAL_ARGS",
    ),
    (
        REPO_ROOT / "tools/scripts/release/package_backend_deployment_kit.sh",
        "APPROVAL_PUBLIC_KEY",
        "APPROVAL_SIGNATURE",
        "LEGAL_ARGS",
    ),
)


class ReleasePresetContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.document = json.loads(PRESETS.read_text(encoding="utf-8"))
        cls.configure = {preset["name"]: preset for preset in cls.document["configurePresets"]}
        cls.build = {preset["name"]: preset for preset in cls.document["buildPresets"]}

    def test_client_release_preset_builds_only_redistributable_client(self):
        preset = self.configure["linux-client-release-gcc16"]
        variables = preset["cacheVariables"]

        self.assertEqual("${sourceDir}/build-linux-client-release-gcc16", preset["binaryDir"])
        self.assertEqual("ON", variables["BUILD_CLIENT"])
        self.assertEqual("OFF", variables["BUILD_SERVER"])
        self.assertEqual("OFF", variables["BUILD_OPS"])
        self.assertEqual("OFF", variables["BUILD_TESTS"])
        self.assertEqual("ON", variables["MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD"])
        self.assertEqual("OFF", variables["MEMOCHAT_ENABLE_LIVE2D_NATIVE"])
        self.assertEqual("", variables["MEMOCHAT_LIVE2D_SDK_ROOT"])
        self.assertEqual("ON", variables["VCPKG_MANIFEST_NO_DEFAULT_FEATURES"])
        self.assertEqual("", variables["VCPKG_MANIFEST_FEATURES"])

    def test_server_release_preset_builds_only_current_backend(self):
        preset = self.configure["linux-server-release-gcc16"]
        variables = preset["cacheVariables"]

        self.assertEqual("${sourceDir}/build-linux-server-release-gcc16", preset["binaryDir"])
        self.assertEqual("OFF", variables["BUILD_CLIENT"])
        self.assertEqual("ON", variables["BUILD_SERVER"])
        self.assertEqual("OFF", variables["BUILD_OPS"])
        self.assertEqual("OFF", variables["BUILD_TESTS"])
        self.assertEqual("ON", variables["MEMOCHAT_ENABLE_GNU_MODULES"])
        self.assertEqual("server", variables["VCPKG_MANIFEST_FEATURES"])

    def test_release_presets_disable_developer_side_effects(self):
        for name in ("linux-client-release-gcc16", "linux-server-release-gcc16"):
            variables = self.configure[name]["cacheVariables"]
            with self.subTest(name=name):
                self.assertEqual("OFF", variables["MEMOCHAT_AUTO_FORMAT"])
                self.assertEqual("OFF", variables["MEMOCHAT_AUTO_DEPLOY_RUNTIME"])
                self.assertEqual("OFF", variables["MEMOCHAT_ENABLE_CPP26_REFLECTION"])
                self.assertEqual(name, self.build[name]["configurePreset"])

        self.assertEqual(
            "OFF",
            self.configure["linux-client-release-gcc16"]["cacheVariables"]["MEMOCHAT_ENABLE_GNU_MODULES"],
        )

    def test_release_presets_limit_parallelism_for_bounded_memory(self):
        for name in ("linux-client-release-gcc16", "linux-server-release-gcc16"):
            environment = self.configure[name]["environment"]
            with self.subTest(name=name):
                self.assertEqual("2", environment["VCPKG_MAX_CONCURRENCY"])
                self.assertEqual("1", environment["CMAKE_BUILD_PARALLEL_LEVEL"])
                self.assertEqual(1, self.build[name]["jobs"])

        release_docs = SERVICE_IMAGE_README.read_text(encoding="utf-8")
        self.assertNotRegex(
            release_docs,
            r"cmake --build --preset linux-(?:client|server)-release-gcc16\s+--parallel",
        )

    def test_packagers_only_forward_explicit_external_legal_approval_inputs(self):
        for packager, key_variable, signature_variable, argument_array in LEGAL_APPROVAL_PACKAGERS:
            with self.subTest(packager=packager.name):
                help_result = subprocess.run(
                    ["bash", str(packager), "--help"],
                    cwd=REPO_ROOT,
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    check=False,
                )
                self.assertEqual(0, help_result.returncode, help_result.stdout)
                self.assertIn("--approval-public-key", help_result.stdout)
                self.assertIn("--approval-signature", help_result.stdout)

                script = packager.read_text(encoding="utf-8")
                self.assertIn(f'{key_variable}=""', script)
                self.assertIn(f'{signature_variable}=""', script)
                self.assertIn("--approval-public-key)", script)
                self.assertIn("--approval-signature)", script)
                self.assertIn(
                    f'{argument_array}+=(--approval-public-key "${key_variable}")',
                    script,
                )
                self.assertIn(
                    f'{argument_array}+=(--approval-signature "${signature_variable}")',
                    script,
                )
                self.assertNotIn(f'{key_variable}="${{', script)
                self.assertNotIn(f'{signature_variable}="${{', script)

                verifier_calls = [
                    line.strip() for line in script.splitlines() if line.lstrip().startswith('"$LEGAL_VERIFIER"')
                ]
                self.assertTrue(verifier_calls)
                for verifier_call in verifier_calls:
                    self.assertIn(f'"${{{argument_array}[@]}}"', verifier_call)

    def test_release_presets_scrub_source_and_dependency_build_paths(self):
        for name in ("linux-client-release-gcc16", "linux-server-release-gcc16"):
            variables = self.configure[name]["cacheVariables"]
            with self.subTest(name=name):
                self.assertEqual("x64-linux-memochat-release", variables["VCPKG_TARGET_TRIPLET"])
                self.assertEqual(
                    "${sourceDir}/cmake/vcpkg-triplets",
                    variables["VCPKG_OVERLAY_TRIPLETS"],
                )
                self.assertEqual(
                    "${sourceDir}/cmake/vcpkg-overlay-ports",
                    variables["VCPKG_OVERLAY_PORTS"],
                )
                self.assertTrue(variables["VCPKG_INSTALLED_DIR"].startswith("$env{VCPKG_ROOT}/"))
                flags = variables["CMAKE_CXX_FLAGS_RELEASE"]
                self.assertIn("-ffile-prefix-map=${sourceDir}=", flags)
                self.assertIn("-fmacro-prefix-map=${sourceDir}=", flags)
                self.assertIn("-ffile-prefix-map=$env{VCPKG_ROOT}=", flags)

        triplet = RELEASE_TRIPLET.read_text(encoding="utf-8")
        self.assertIn("VCPKG_C_FLAGS_RELEASE", triplet)
        self.assertIn("VCPKG_CXX_FLAGS_RELEASE", triplet)
        self.assertIn("-ffile-prefix-map=${_memochat_vcpkg_root}=", triplet)
        self.assertIn("-fmacro-prefix-map=${_memochat_vcpkg_root}=", triplet)

        msquic_portfile = MSQUIC_PORTFILE.read_text(encoding="utf-8")
        self.assertIn("MODULESDIR=$(libdir)/ossl-modules", msquic_portfile)
        self.assertIn("MODULESDIR=/usr/lib/x86_64-linux-gnu/ossl-modules", msquic_portfile)

    def test_nghttp2_overlay_fetches_the_pinned_source_with_integrity_check(self):
        portfile = NGHTTP2_PORTFILE.read_text(encoding="utf-8")
        manifest = json.loads(NGHTTP2_MANIFEST.read_text(encoding="utf-8"))

        self.assertEqual("1.68.0", manifest["version"])
        self.assertEqual("MIT", manifest["license"])
        self.assertIn("vcpkg-cmake", [dependency["name"] for dependency in manifest["dependencies"]])
        self.assertIn("vcpkg_from_github(", portfile)
        self.assertIn("REPO nghttp2/nghttp2", portfile)
        self.assertIn('REF "v${VERSION}"', portfile)
        self.assertRegex(portfile, r"SHA512 [0-9a-f]{128}")
        self.assertNotIn("pre-downloaded tarball", portfile)
        self.assertNotIn("tarball not found", portfile)

    def test_release_crypto_and_database_ports_use_stable_runtime_paths(self):
        openssl_manifest = json.loads((OPENSSL_PORT / "vcpkg.json").read_text(encoding="utf-8"))
        openssl_portfile = (OPENSSL_PORT / "portfile.cmake").read_text(encoding="utf-8")
        openssl_unix_portfile = (OPENSSL_PORT / "unix/portfile.cmake").read_text(encoding="utf-8")
        libpq_manifest = json.loads((LIBPQ_PORT / "vcpkg.json").read_text(encoding="utf-8"))
        libpq_portfile = (LIBPQ_PORT / "portfile.cmake").read_text(encoding="utf-8")

        self.assertEqual("3.6.0", openssl_manifest["version"])
        self.assertEqual(4, openssl_manifest["port-version"])
        self.assertIn("vcpkg_replace_string(", openssl_portfile)
        self.assertIn('"--openssldir=/etc/ssl"', openssl_unix_portfile)
        self.assertIn("ENGINESDIR=/usr/lib/x86_64-linux-gnu/engines-3", openssl_portfile)
        self.assertIn("MODULESDIR=/usr/lib/x86_64-linux-gnu/ossl-modules", openssl_portfile)
        self.assertIn("-f(?:file|macro)-prefix-map", openssl_portfile)

        self.assertEqual("16.9", libpq_manifest["version"])
        self.assertEqual(3, libpq_manifest["port-version"])
        self.assertIn("--sysconfdir=/etc", libpq_portfile)

        for portfile in (
            openssl_portfile,
            openssl_unix_portfile,
            libpq_portfile,
        ):
            self.assertNotIn("/data/vcpkg", portfile)
            self.assertNotIn("/root/code", portfile)

    def test_mongo_driver_does_not_publish_build_flags_in_handshake_metadata(self):
        manifest = json.loads((MONGO_C_DRIVER_PORT / "vcpkg.json").read_text(encoding="utf-8"))
        portfile = (MONGO_C_DRIVER_PORT / "portfile.cmake").read_text(encoding="utf-8")
        patch = (MONGO_C_DRIVER_PORT / "redact-build-flags.patch").read_text(encoding="utf-8")

        self.assertEqual("1.30.6", manifest["version"])
        self.assertIn("redact-build-flags.patch", portfile)
        self.assertIn('set (MONGOC_USER_SET_CFLAGS "")', patch)
        self.assertIn('set (MONGOC_USER_SET_LDFLAGS "")', patch)


if __name__ == "__main__":
    unittest.main()
