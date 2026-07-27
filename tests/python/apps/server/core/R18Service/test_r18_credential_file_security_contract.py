import json
import os
import shlex
import stat
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

STORE = repo_root() / "apps/server/core/R18Service/domain/services/r18/R18SourceCredentialStore.cpp"

RUNTIME_HARNESS = r"""
#include "r18/R18SourceCredentialStore.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
constexpr int kUid = 731;
constexpr const char* kKey = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
constexpr const char* kWrongKey = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
constexpr const char* kSource = "credential-runtime-source";
constexpr const char* kUsername = "credential-runtime-user";
constexpr const char* kPassword = "credential-runtime-password";

std::filesystem::path CredentialPath(const std::filesystem::path& root, int uid) {
    return root / "data" / "r18" / "credentials" / (std::to_string(uid) + ".json");
}

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool WriteFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << content;
    output.close();
    return output.good();
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const std::string mode = argv[1];
    const std::filesystem::path root = argv[2];
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    std::filesystem::current_path(root, ec);
    if (ec) return 3;

    if (mode == "write") {
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kKey, 1);
        std::string error;
        return memochat::r18::R18SourceCredentialStore::Instance().UpsertLogin(
                   kUid, kSource, kUsername, kPassword, &error)
                   ? 0
                   : 10;
    }
    if (mode == "update") {
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kKey, 1);
        std::string error;
        return memochat::r18::R18SourceCredentialStore::Instance().UpdateSession(
                   kUid, kSource, "session-secret", "cookie-secret", "authenticated", "", &error)
                   ? 0
                   : 20;
    }
    if (mode == "read") {
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kKey, 1);
        const auto credential = memochat::r18::R18SourceCredentialStore::Instance().Get(kUid, kSource);
        return credential && credential->username == kUsername && credential->password == kPassword &&
                       credential->session_token == "session-secret" && credential->session_cookie == "cookie-secret"
                   ? 0
                   : 30;
    }
    if (mode == "wrong-key-write") {
        const auto path = CredentialPath(root, kUid);
        const std::string before = ReadFile(path);
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kWrongKey, 1);
        std::string error;
        const bool saved = memochat::r18::R18SourceCredentialStore::Instance().UpsertLogin(
            kUid, kSource, kUsername, "must-not-save", &error);
        return !saved && ReadFile(path) == before ? 0 : 40;
    }
    if (mode == "aad") {
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kKey, 1);
        std::filesystem::copy_file(CredentialPath(root, kUid), CredentialPath(root, kUid + 1),
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) return 50;
        return memochat::r18::R18SourceCredentialStore::Instance().Get(kUid + 1, kSource) ? 51 : 0;
    }
    if (mode == "rollback") {
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kKey, 1);
        auto& store = memochat::r18::R18SourceCredentialStore::Instance();
        const auto before = store.Get(kUid, kSource);
        if (!before) return 60;
        const auto temporary = std::filesystem::path(CredentialPath(root, kUid).string() + ".tmp");
        std::filesystem::create_directories(temporary, ec);
        std::ofstream blocker(temporary / "blocker");
        blocker << "block";
        blocker.close();
        std::string error;
        const bool saved = store.UpdateSession(kUid, kSource, "must-not-commit", "", "authenticated", "", &error);
        const auto after = store.Get(kUid, kSource);
        std::filesystem::remove_all(temporary, ec);
        return !saved && after && after->session_token == before->session_token ? 0 : 61;
    }
    if (mode == "legacy") {
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kKey, 1);
        const auto path = CredentialPath(root, kUid + 2);
        std::filesystem::create_directories(path.parent_path(), ec);
        const std::string plaintext = "[{\"source_id\":\"old-plaintext\",\"password\":\"must-not-load\"}]";
        if (!WriteFile(path, plaintext)) return 70;
        std::string error;
        const bool saved = memochat::r18::R18SourceCredentialStore::Instance().UpsertLogin(
            kUid + 2, kSource, kUsername, kPassword, &error);
        return !saved && error == "credential file is not an encrypted v1 envelope" && ReadFile(path) == plaintext
                   ? 0
                   : 71;
    }
    if (mode == "tamper") {
        setenv("MEMOCHAT_R18_CREDENTIAL_MASTER_KEY", kKey, 1);
        const auto path = CredentialPath(root, kUid);
        std::string tampered = ReadFile(path);
        const std::string marker = "\"ciphertext\":\"";
        const auto ciphertext = tampered.find(marker);
        if (ciphertext == std::string::npos) return 80;
        const auto byte = ciphertext + marker.size();
        tampered[byte] = tampered[byte] == '0' ? '1' : '0';
        if (!WriteFile(path, tampered)) return 81;
        std::string error;
        const bool saved = memochat::r18::R18SourceCredentialStore::Instance().UpsertLogin(
            kUid, kSource, kUsername, "must-not-save", &error);
        return !saved && ReadFile(path) == tampered ? 0 : 82;
    }
    return 4;
}
"""


class R18CredentialFileSecurityContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = STORE.read_text(encoding="utf-8")

    def test_credential_directory_is_owner_only(self):
        self.assertIn("std::filesystem::perms::owner_all", self.source)
        self.assertIn("std::filesystem::perm_options::replace", self.source)

    def test_existing_and_written_credential_files_are_owner_read_write(self):
        owner_file_permissions = "std::filesystem::perms::owner_read | std::filesystem::perms::owner_write"
        self.assertIn(owner_file_permissions, self.source)
        self.assertIn("!TightenCredentialFilePermissions(temporary_path)", self.source)
        self.assertIn("!TightenCredentialFilePermissions(path)", self.source)

    def test_writes_are_atomic_and_storage_errors_are_propagated(self):
        self.assertIn('temporary_path += ".tmp"', self.source)
        self.assertIn("std::filesystem::rename(temporary_path, path, ec)", self.source)
        self.assertIn("MoveFileExW(temporary_path.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING", self.source)
        self.assertNotIn("std::filesystem::remove(path, ec)", self.source)
        self.assertIn("bool R18SourceCredentialStore::SaveUidLocked", self.source)
        self.assertGreaterEqual(self.source.count("return SaveUidOrReloadLocked(uid, error);"), 6)

    def test_symlinked_storage_paths_are_rejected(self):
        self.assertIn("std::filesystem::symlink_status(root_, ec)", self.source)
        self.assertIn("std::filesystem::symlink_status(path, ec)", self.source)
        self.assertGreaterEqual(self.source.count("std::filesystem::is_symlink(status)"), 2)

    def test_credentials_are_encrypted_with_an_injected_aead_key(self):
        for token in (
            "MEMOCHAT_R18_CREDENTIAL_MASTER_KEY",
            "kMasterKeyBytes = 32",
            "kGcmNonceBytes = 12",
            "kGcmTagBytes = 16",
            "EVP_aes_256_gcm()",
            "EVP_EncryptInit_ex",
            "EVP_DecryptInit_ex",
            "EVP_CTRL_GCM_GET_TAG",
            "EVP_CTRL_GCM_SET_TAG",
            "RAND_bytes",
            'envelope["version"]',
            'envelope["ciphertext"]',
        ):
            with self.subTest(token=token):
                self.assertIn(token, self.source)

        self.assertNotIn("out << json::glaze_stringify(arr)", self.source)

    def test_envelope_authentication_is_bound_to_the_expected_uid(self):
        self.assertIn('"memochat:r18:credentials:v1:uid=" + std::to_string(uid)', self.source)
        self.assertIn("EVP_EncryptUpdate(context", self.source)
        self.assertIn("EVP_DecryptUpdate(context", self.source)

    def test_plaintext_credential_files_are_not_migrated_or_loaded(self):
        self.assertIn("credential file is not an encrypted v1 envelope", self.source)
        self.assertIn("root.is_object()", self.source)
        self.assertGreaterEqual(
            self.source.count("if (!LoadUidLocked(uid, error))"),
            6,
            "a failed decrypt/load must not fall through and overwrite the original file",
        )

    def test_cached_credentials_cannot_overwrite_a_changed_backing_envelope(self):
        self.assertIn("BackingFileMatchesLoadedSnapshotLocked", self.source)
        self.assertIn("credential file changed after it was loaded", self.source)
        self.assertIn("loaded_envelopes_[uid] = content", self.source)
        self.assertIn("loaded_envelopes_[uid] = serialized_envelope", self.source)
        self.assertNotIn(
            "if (by_uid_.find(uid) != by_uid_.end())\n        return true;",
            self.source,
            "every operation must re-authenticate the current on-disk envelope",
        )

    def test_failed_writes_discard_uncommitted_in_memory_credentials(self):
        save_or_reload = self.source.split("bool R18SourceCredentialStore::SaveUidOrReloadLocked", 1)[1]
        save_or_reload = save_or_reload.split("JsonValue R18SourceCredentialStore::ToPublicJson", 1)[0]
        self.assertIn("if (SaveUidLocked(uid, error))", save_or_reload)
        self.assertIn("LoadUidLocked(uid, nullptr);", save_or_reload)


class R18CredentialFileSecurityRuntimeTests(unittest.TestCase):
    def _configured_build(self):
        root = repo_root()
        candidates = [root / "build-linux-full-gcc16", root / "build-linux-server-release-gcc16"]
        candidates.extend(sorted(root.glob("build-*gcc16")))
        seen = set()
        for build_dir in candidates:
            build_dir = build_dir.resolve()
            if build_dir in seen:
                continue
            seen.add(build_dir)
            compile_database = build_dir / "compile_commands.json"
            cache = build_dir / "CMakeCache.txt"
            if not compile_database.is_file() or not cache.is_file():
                continue
            entries = json.loads(compile_database.read_text(encoding="utf-8"))
            entry = next((item for item in entries if Path(item["file"]).resolve() == STORE.resolve()), None)
            if entry is not None:
                return build_dir, entry, cache
        self.skipTest("a configured GCC 16 server build is required for the runtime credential-store test")

    def _compile_harness(self, temporary_dir):
        _, entry, cache = self._configured_build()
        command = shlex.split(entry["command"])
        compiler = command[0]
        flags = []
        index = 1
        while index < len(command):
            token = command[index]
            if token == "-isystem" and index + 1 < len(command):
                flags.extend((token, command[index + 1]))
                index += 2
                continue
            if token.startswith(("-D", "-I", "-std=")) or token in {"-fno-exceptions", "-freflection"}:
                flags.append(token)
            index += 1

        crypto_library = None
        for line in cache.read_text(encoding="utf-8").splitlines():
            if line.startswith("OPENSSL_CRYPTO_LIBRARY:FILEPATH="):
                crypto_library = Path(line.split("=", 1)[1])
                break
        if crypto_library is None or not crypto_library.is_file():
            self.skipTest("the configured server build does not expose its OpenSSL crypto library")

        harness = Path(temporary_dir) / "r18_credential_store_runtime.cpp"
        executable = Path(temporary_dir) / "r18_credential_store_runtime"
        harness.write_text(textwrap.dedent(RUNTIME_HARNESS), encoding="utf-8")
        compile_result = subprocess.run(
            [
                compiler,
                *flags,
                str(harness),
                str(STORE),
                str(crypto_library),
                "-ldl",
                "-pthread",
                "-o",
                str(executable),
            ],
            check=False,
            capture_output=True,
            text=True,
            timeout=120,
        )
        self.assertEqual(compile_result.returncode, 0, compile_result.stderr)
        return executable

    def test_runtime_aead_round_trip_and_fail_closed_matrix(self):
        if os.name != "posix":
            self.skipTest("the release runtime credential-store test currently targets Linux")

        with tempfile.TemporaryDirectory(prefix="memochat-r18-credential-runtime-") as temporary_dir:
            executable = self._compile_harness(temporary_dir)
            data_root = Path(temporary_dir) / "runtime"

            def run(mode):
                result = subprocess.run(
                    [str(executable), mode, str(data_root)],
                    check=False,
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
                self.assertEqual(result.returncode, 0, f"{mode}: {result.stdout}\n{result.stderr}")

            run("write")
            credential_directory = data_root / "data/r18/credentials"
            credential_file = credential_directory / "731.json"
            first_envelope_text = credential_file.read_text(encoding="utf-8")
            first_envelope = json.loads(first_envelope_text)
            self.assertEqual(stat.S_IMODE(credential_directory.stat().st_mode), 0o700)
            self.assertEqual(stat.S_IMODE(credential_file.stat().st_mode), 0o600)
            self.assertEqual(first_envelope["version"], 1)
            self.assertEqual(first_envelope["algorithm"], "AES-256-GCM")
            self.assertEqual(len(bytes.fromhex(first_envelope["nonce"])), 12)
            self.assertEqual(len(bytes.fromhex(first_envelope["tag"])), 16)
            self.assertNotIn("credential-runtime-user", first_envelope_text)
            self.assertNotIn("credential-runtime-password", first_envelope_text)

            run("update")
            second_envelope_text = credential_file.read_text(encoding="utf-8")
            second_envelope = json.loads(second_envelope_text)
            self.assertNotEqual(first_envelope["nonce"], second_envelope["nonce"])
            self.assertNotIn("session-secret", second_envelope_text)
            self.assertNotIn("cookie-secret", second_envelope_text)

            for mode in ("read", "wrong-key-write", "aad", "rollback", "legacy", "tamper"):
                with self.subTest(mode=mode):
                    run(mode)


if __name__ == "__main__":
    unittest.main()
