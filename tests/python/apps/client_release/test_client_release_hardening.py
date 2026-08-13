from __future__ import annotations

import configparser
import json
import os
import re
import shutil
import stat
import subprocess
import tempfile
import textwrap
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[4]
CLIENT_ROOT = REPO_ROOT / "apps/client/desktop/MemoChat-qml"
LOGGING_HEADER = CLIENT_ROOT / "app/bootstrap/MainLogging.h"
LOGGING_SOURCE = CLIENT_ROOT / "app/bootstrap/MainLogging.cpp"
QML_ENGINE_SOURCE = CLIENT_ROOT / "app/bootstrap/MainQmlEngineSetup.cpp"
CLIENT_CMAKE = CLIENT_ROOT / "CMakeLists.txt"
CLIENT_CORE_CMAKE = CLIENT_ROOT / "core/CMakeLists.txt"
CLIENT_MAIN = CLIENT_ROOT / "app/bootstrap/main.cpp"
CLIENT_CONFIG = CLIENT_ROOT / "core/config.ini"
CLIENT_NETWORK_ROOT = CLIENT_ROOT
HTTP_REQUEST_UTILS = CLIENT_ROOT / "core/network/HttpMgrRequestUtils.cpp"
TELEMETRY_HEADER = CLIENT_ROOT / "core/network/TelemetryUtils.h"
TELEMETRY_SOURCE = CLIENT_ROOT / "core/network/TelemetryUtils.cpp"
MAIN_RUNTIME_CONFIG = CLIENT_ROOT / "app/bootstrap/MainRuntimeConfig.cpp"
CHAT_CONNECTION_POLICY = CLIENT_ROOT / "app/connection/AppChatConnectionPolicy.cpp"
LOGIN_RESPONSE_HANDLER = CLIENT_ROOT / "app/session/SessionAuthCoordinatorLoginResponse.cpp"
LIVE2D_CORE = CLIENT_ROOT / "live2d/rendering/Live2DCoreRenderer.cpp"
AUTH_CREDENTIAL_STORE = CLIENT_ROOT / "features/auth/service/AuthCredentialStore.h"
RELEASE_RESOURCE_VALIDATOR = CLIENT_ROOT / "cmake/ValidateClientReleaseResources.cmake"
PACKAGE_SCRIPT = REPO_ROOT / "tools/scripts/release/package_linux_client.sh"
RELEASE_TREE_VERIFIER = REPO_ROOT / "tools/scripts/release/verify_release_tree.sh"
RELEASE_POLICY_MARKER = "MEMOCHAT_RELEASE_POLICY:v1;distributable=1;live2d_native=0;restricted_assets=0"
PAYLOAD_LOG_SOURCES = (
    CLIENT_ROOT / "core/network/tcpmgr.cpp",
    CLIENT_ROOT / "core/network/ChatMessageDispatcherAuth.cpp",
    CLIENT_ROOT / "core/network/ChatMessageDispatcherContacts.cpp",
    CLIENT_ROOT / "core/network/ChatMessageDispatcherPrivate.cpp",
    CLIENT_ROOT / "features/moments/controller/MomentsControllerFeedResponses.cpp",
    CLIENT_ROOT / "features/moments/controller/MomentsControllerPostResponses.cpp",
)


def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, check=True, text=True, capture_output=True, **kwargs)


def release_probe_source(program: str) -> str:
    return (
        '__attribute__((used, section(".note.memochat.release"))) '
        f'static const char release_policy[] = "{RELEASE_POLICY_MARKER}";\n'
        f"{program}\n"
    )


def test_logging_exposes_a_testable_redaction_boundary() -> None:
    header = LOGGING_HEADER.read_text(encoding="utf-8")

    assert "redactSensitiveLogMessage" in header


def test_release_client_never_logs_raw_network_or_social_payloads() -> None:
    forbidden = re.compile(
        r"q(?:Debug|Info|Warning|Critical)\s*\(\s*\).*"
        r"(?:<<\s*(?:data|block|jsonObj|res)\s*(?:;|<<)|<<\s*res\.left\s*\()"
    )
    violations = {
        str(path.relative_to(CLIENT_ROOT)): match.group(0)
        for path in PAYLOAD_LOG_SOURCES
        for match in forbidden.finditer(path.read_text(encoding="utf-8"))
    }

    assert violations == {}
    assert "QT_NO_DEBUG_OUTPUT" in CLIENT_CMAKE.read_text(encoding="utf-8")
    core_cmake = CLIENT_CORE_CMAKE.read_text(encoding="utf-8")
    assert "QT_NO_DEBUG_OUTPUT" in core_cmake
    assert "MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD=$<BOOL:" in core_cmake


def test_release_client_routes_http_features_through_the_loopback_gateway() -> None:
    config = configparser.ConfigParser()
    config.read(CLIENT_CONFIG, encoding="utf-8")

    assert config["GateServer"]["scheme"] == "https"
    assert config["GateServer"]["port"] == "8443"
    assert config["GateServer"]["http_port"] == "8443"
    assert config["AI"]["BaseUrl"] == ""
    assert config["Media"]["BaseUrl"] == ""


def test_release_client_telemetry_is_opt_in_and_https_only() -> None:
    config = configparser.ConfigParser()
    config.read(CLIENT_CONFIG, encoding="utf-8")

    assert config.getboolean("Telemetry", "Enabled") is False
    assert config["Telemetry"]["OtlpEndpoint"] == ""
    assert config.getboolean("Telemetry", "ExportLogs") is False
    assert config.getboolean("Telemetry", "ExportTraces") is False

    header = TELEMETRY_HEADER.read_text(encoding="utf-8")
    source = TELEMETRY_SOURCE.read_text(encoding="utf-8")
    assert "bool enabled = false;" in header
    assert "bool exportLogs = false;" in header
    assert "bool exportTraces = false;" in header
    assert "#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD" in source
    assert 'endpoint.scheme().compare(QStringLiteral("https")' in source
    assert "endpoint.userName().isEmpty()" in source
    assert "endpoint.password().isEmpty()" in source
    assert "configureSecureNetworkRequest(request)" in source


def test_release_client_never_disables_tls_certificate_validation() -> None:
    forbidden = (
        "QSslSocket::VerifyNone",
        "ignoreSslErrors",
        "QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION",
    )
    source_files = tuple(CLIENT_NETWORK_ROOT.rglob("*.cpp")) + tuple(CLIENT_NETWORK_ROOT.rglob("*.h"))

    violations = {
        str(path.relative_to(CLIENT_ROOT)): token
        for path in source_files
        for token in forbidden
        if token in path.read_text(encoding="utf-8", errors="ignore")
    }

    assert violations == {}


def test_gate_retry_policy_does_not_construct_a_plaintext_fallback() -> None:
    source = HTTP_REQUEST_UTILS.read_text(encoding="utf-8")

    assert 'withGateEndpoint(url, QStringLiteral("http")' not in source
    assert "Never downgrade a credential-bearing HTTPS request" in source
    assert "request.setUrl(QUrl())" in source
    assert "Distributable client rejected a non-HTTPS" in source


def test_distributable_client_never_accepts_plaintext_chat_fallback() -> None:
    policy = CHAT_CONNECTION_POLICY.read_text(encoding="utf-8")
    login_response = LOGIN_RESPONSE_HANDLER.read_text(encoding="utf-8")

    assert "#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD" in policy
    assert "constexpr ChatTransportKind preferredTransport = ChatTransportKind::Quic" in policy
    assert "constexpr ChatTransportKind fallbackTransport = ChatTransportKind::Quic" in policy
    assert "Q_UNUSED(snapshot);" in policy
    assert "#if MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD" in login_response
    assert "if (*endpointTransport != ChatTransportKind::Quic)" in login_response
    assert "server_info.LoginTicket.trimmed().isEmpty() || !hasSecureChatEndpoint" in login_response


@pytest.fixture(scope="module")
def release_runtime_config_probe(tmp_path_factory: pytest.TempPathFactory) -> Path:
    compiler = shutil.which("g++")
    qtpaths = shutil.which("qtpaths6")
    if not compiler or not qtpaths:
        pytest.skip("g++ and qtpaths6 are required for the release config probe")

    qt_root = Path(run([qtpaths, "--query", "QT_INSTALL_PREFIX"]).stdout.strip())
    probe_dir = tmp_path_factory.mktemp("release-runtime-config-probe")
    probe_source = probe_dir / "probe.cpp"
    probe_source.write_text(
        textwrap.dedent(
            """
            #include "MainRuntimeConfig.h"

            #include <QString>

            QString gate_url_prefix;
            QString gate_media_url_prefix;

            int main(int argc, char* argv[])
            {
                if (argc != 2)
                {
                    return 2;
                }
                if (!configureGateUrlPrefixes(QString::fromLocal8Bit(argv[1])))
                {
                    return 3;
                }
                return gate_url_prefix.startsWith(QStringLiteral("https://")) &&
                               gate_media_url_prefix.startsWith(QStringLiteral("https://"))
                    ? 0
                    : 4;
            }
            """
        ),
        encoding="utf-8",
    )
    probe = probe_dir / "release_runtime_config_probe"
    run(
        [
            compiler,
            "-std=c++23",
            "-fPIC",
            "-DMEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD=1",
            "-I",
            str(MAIN_RUNTIME_CONFIG.parent),
            "-I",
            str(CLIENT_ROOT / "core/common"),
            str(probe_source),
            str(MAIN_RUNTIME_CONFIG),
            "-o",
            str(probe),
            f"-I{qt_root / 'include'}",
            f"-I{qt_root / 'include/QtCore'}",
            f"-I{qt_root / 'include/QtGui'}",
            f"-I{qt_root / 'include/QtWidgets'}",
            f"-I{qt_root / 'mkspecs/linux-g++'}",
            "-DQT_CORE_LIB",
            "-DQT_GUI_LIB",
            "-DQT_WIDGETS_LIB",
            f"-L{qt_root / 'lib'}",
            "-lQt6Widgets",
            "-lQt6Gui",
            "-lQt6Core",
            f"-Wl,-rpath,{qt_root / 'lib'}",
        ]
    )
    return probe


def test_distributable_runtime_config_accepts_only_https_endpoints(
    release_runtime_config_probe: Path, tmp_path: Path
) -> None:
    secure = tmp_path / "secure.ini"
    secure.write_text(
        "[GateServer]\nhost=127.0.0.1\nport=8443\nscheme=https\n"
        "[AI]\nBaseUrl=\n[Media]\nBaseUrl=\n[Call]\nCallBaseUrl=https://127.0.0.1:8443\n",
        encoding="utf-8",
    )
    assert subprocess.run([str(release_runtime_config_probe), str(secure)], check=False).returncode == 0

    for name, replacement in (
        ("gate", "scheme=http"),
        ("ai", "scheme=https\n[AI]\nBaseUrl=http://127.0.0.1:8080"),
        ("media", "scheme=https\n[Media]\nBaseUrl=http://127.0.0.1:9000"),
    ):
        config = tmp_path / f"{name}.ini"
        config.write_text(
            f"[GateServer]\nhost=127.0.0.1\nport=8443\n{replacement}\n",
            encoding="utf-8",
        )
        assert subprocess.run([str(release_runtime_config_probe), str(config)], check=False).returncode == 3


@pytest.fixture(scope="module")
def logging_probe(tmp_path_factory: pytest.TempPathFactory) -> Path:
    compiler = shutil.which("g++")
    qtpaths = shutil.which("qtpaths6")
    if not compiler or not qtpaths:
        pytest.skip("g++ and qtpaths6 are required for the Qt logging probe")

    qt_root = Path(run([qtpaths, "--query", "QT_INSTALL_PREFIX"]).stdout.strip())
    qt_flags = [
        f"-I{qt_root / 'include'}",
        f"-I{qt_root / 'include/QtCore'}",
        f"-I{qt_root / 'mkspecs/linux-g++'}",
        "-DQT_CORE_LIB",
        f"-L{qt_root / 'lib'}",
        "-lQt6Core",
        f"-Wl,-rpath,{qt_root / 'lib'}",
    ]
    probe_dir = tmp_path_factory.mktemp("logging-probe")
    probe_source = probe_dir / "logging_probe.cpp"
    probe_source.write_text(
        textwrap.dedent(
            """
            #include "MainLogging.h"

            #include <QCoreApplication>
            #include <QString>

            int main(int argc, char* argv[])
            {
                QCoreApplication app(argc, argv);
                if (argc != 4)
                {
                    return 2;
                }
                loadRuntimeLogConfig(QString::fromLocal8Bit(argv[1]),
                                     QString::fromLocal8Bit(argv[2]));
                fileMessageHandler(QtWarningMsg, QMessageLogContext{},
                                   QString::fromLocal8Bit(argv[3]));
                return 0;
            }
            """
        ),
        encoding="utf-8",
    )
    probe = probe_dir / "logging_probe"
    run(
        [
            compiler,
            "-std=c++23",
            "-fPIC",
            "-I",
            str(LOGGING_HEADER.parent),
            str(probe_source),
            str(LOGGING_SOURCE),
            "-o",
            str(probe),
            *qt_flags,
        ]
    )
    return probe


@pytest.mark.parametrize(
    ("message", "secret", "preserved"),
    [
        (
            "GET https://localhost/media?id=42&access_token=media-secret&size=full",
            "media-secret",
            "size=full",
        ),
        ("Authorization: Bearer eyJhbGciOiJIUzI1NiJ9", "eyJhbGciOiJIUzI1NiJ9", "Authorization"),
        ("Cookie: session=logged-in; theme=dark", "logged-in", "Cookie"),
        ('request={"password":"hunter2","event":"login"}', "hunter2", '"event":"login"'),
        ("api_key=local-api-key request_id=abc", "local-api-key", "request_id=abc"),
        ("register user@example.invalid request_id=abc", "user@example.invalid", "request_id=abc"),
        ("clientSecret=oauth-client-secret request_id=abc", "oauth-client-secret", "request_id=abc"),
        ('request={"jwt_secret":"jwt-signing-secret","event":"auth"}', "jwt-signing-secret", '"event":"auth"'),
        ("hmac-key=hmac-signing-key request_id=abc", "hmac-signing-key", "request_id=abc"),
        ("accessKey=storage-access-key request_id=abc", "storage-access-key", "request_id=abc"),
        ("secret_key=storage-secret-key request_id=abc", "storage-secret-key", "request_id=abc"),
        ("loginTicket=one-time-ticket request_id=abc", "one-time-ticket", "request_id=abc"),
        ("verify-code=482931 request_id=abc", "482931", "request_id=abc"),
        ("privateKey=private-signing-material request_id=abc", "private-signing-material", "request_id=abc"),
        ("jwt_key=jwt-key-material request_id=abc", "jwt-key-material", "request_id=abc"),
        ("turn-credential=turn-password-material request_id=abc", "turn-password-material", "request_id=abc"),
        (
            "connect mongodb://db-user:uri-password@127.0.0.1:27017/memo request_id=abc",
            "uri-password",
            "127.0.0.1:27017/memo",
        ),
    ],
)
def test_logging_redacts_sensitive_values_before_file_output(
    logging_probe: Path,
    tmp_path: Path,
    message: str,
    secret: str,
    preserved: str,
) -> None:
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nDir=logs\nToConsole=false\nRedact=true\n", encoding="utf-8")

    run([str(logging_probe), str(config), str(tmp_path), message])

    log_file = next((tmp_path / "logs").glob("MemoChatQml_*.json"))
    payload = json.loads(log_file.read_text(encoding="utf-8").splitlines()[-1])
    assert secret not in payload["message"]
    assert "[REDACTED]" in payload["message"]
    assert preserved in payload["message"]


def test_logging_redaction_config_can_be_explicitly_disabled(logging_probe: Path, tmp_path: Path) -> None:
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nDir=logs\nToConsole=false\nRedact=false\n", encoding="utf-8")
    message = "token=diagnostic-token request_id=abc"

    run([str(logging_probe), str(config), str(tmp_path), message])

    log_file = next((tmp_path / "logs").glob("MemoChatQml_*.json"))
    payload = json.loads(log_file.read_text(encoding="utf-8").splitlines()[-1])
    assert payload["message"] == message


def test_logging_does_not_redact_public_key_material(logging_probe: Path, tmp_path: Path) -> None:
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nDir=logs\nToConsole=false\nRedact=true\n", encoding="utf-8")
    message = "public_key=ssh-ed25519-public-material request_id=abc"

    run([str(logging_probe), str(config), str(tmp_path), message])

    log_file = next((tmp_path / "logs").glob("MemoChatQml_*.json"))
    payload = json.loads(log_file.read_text(encoding="utf-8").splitlines()[-1])
    assert payload["message"] == message


@pytest.mark.skipif(os.name != "posix", reason="Unix owner-only mode contract")
def test_logging_files_are_owner_only(logging_probe: Path, tmp_path: Path) -> None:
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nDir=logs\nToConsole=false\nRedact=true\n", encoding="utf-8")

    run([str(logging_probe), str(config), str(tmp_path), "ordinary message"])

    log_dir = tmp_path / "logs"
    log_file = next(log_dir.glob("MemoChatQml_*.json"))
    assert stat.S_IMODE(log_dir.stat().st_mode) == 0o700
    assert stat.S_IMODE(log_file.stat().st_mode) == 0o600


@pytest.mark.skipif(os.name != "posix", reason="Unix owner-only mode contract")
def test_logging_refuses_broad_existing_directory_without_chmod(logging_probe: Path, tmp_path: Path) -> None:
    log_dir = tmp_path / "shared-logs"
    log_dir.mkdir(mode=0o750)
    config = tmp_path / "config.ini"
    config.write_text(
        f"[Log]\nDir={log_dir}\nToConsole=true\nRedact=true\n",
        encoding="utf-8",
    )

    result = run([str(logging_probe), str(config), str(tmp_path), "token=directory-secret"])

    assert stat.S_IMODE(log_dir.stat().st_mode) == 0o750
    assert not tuple(log_dir.iterdir())
    assert "directory-secret" not in result.stderr
    assert result.stderr.count("owner-only") == 1


@pytest.mark.skipif(not Path("/proc").is_dir(), reason="requires a non-chmodable procfs directory")
def test_logging_permission_failure_keeps_redacted_console_output(logging_probe: Path, tmp_path: Path) -> None:
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nDir=/proc\nToConsole=true\nRedact=true\n", encoding="utf-8")
    secret = "registered@example.invalid"

    result = run([str(logging_probe), str(config), str(tmp_path), f"email={secret}"])

    assert secret not in result.stderr
    assert "[REDACTED]" in result.stderr
    assert result.stderr.count("owner-only") == 1


def test_qml_warnings_share_the_global_qt_logging_path() -> None:
    source = QML_ENGINE_SOURCE.read_text(encoding="utf-8")
    main_source = (CLIENT_ROOT / "app/bootstrap/main.cpp").read_text(encoding="utf-8")

    assert "qWarning().noquote()" in source
    assert "qInstallMessageHandler(fileMessageHandler)" in main_source


@pytest.mark.skipif(os.name != "posix", reason="Unix owner-only mode contract")
def test_runtime_email_cache_is_owner_only_and_never_persists_password(tmp_path: Path) -> None:
    source = AUTH_CREDENTIAL_STORE.read_text(encoding="utf-8")
    prepare_call = source.index("preparePrivateSettingsStorage(settings)")
    first_write = source.index("settings.setValue")
    assert prepare_call < first_write
    assert "O_NOFOLLOW" in source

    compiler = shutil.which("g++")
    qtpaths = shutil.which("qtpaths6")
    if not compiler or not qtpaths:
        pytest.skip("g++ and qtpaths6 are required for the credential-store probe")

    qt_root = Path(run([qtpaths, "--query", "QT_INSTALL_PREFIX"]).stdout.strip())
    probe_source = tmp_path / "credential_store_probe.cpp"
    probe_source.write_text(
        textwrap.dedent(
            """
            #include "AuthCredentialStore.h"

            #include <QCoreApplication>

            int main(int argc, char* argv[])
            {
                QCoreApplication app(argc, argv);
                AuthCredentialStore store;
                store.saveLoginCredential(QStringLiteral("release-test@example.invalid"),
                                          QStringLiteral("must-not-persist"));
                return 0;
            }
            """
        ),
        encoding="utf-8",
    )
    probe = tmp_path / "credential_store_probe"
    run(
        [
            compiler,
            "-std=c++23",
            "-fPIC",
            "-I",
            str(AUTH_CREDENTIAL_STORE.parent),
            str(probe_source),
            "-o",
            str(probe),
            f"-I{qt_root / 'include'}",
            f"-I{qt_root / 'include/QtCore'}",
            f"-I{qt_root / 'mkspecs/linux-g++'}",
            "-DQT_CORE_LIB",
            f"-L{qt_root / 'lib'}",
            "-lQt6Core",
            f"-Wl,-rpath,{qt_root / 'lib'}",
        ]
    )

    config_home = tmp_path / "config-home"
    probe_env = os.environ.copy()
    probe_env["XDG_CONFIG_HOME"] = str(config_home)
    run([str(probe)], env=probe_env)

    settings_files = tuple(config_home.rglob("*"))
    regular_files = tuple(path for path in settings_files if path.is_file())
    assert len(regular_files) == 1
    settings_file = regular_files[0]
    settings_text = settings_file.read_text(encoding="utf-8")
    assert "release-test@example.invalid" in settings_text
    assert "must-not-persist" not in settings_text
    assert stat.S_IMODE(settings_file.stat().st_mode) == 0o600
    assert stat.S_IMODE(settings_file.parent.stat().st_mode) == 0o700

    symlink_config_home = tmp_path / "symlink-config-home"
    symlink_settings_dir = symlink_config_home / "MemoChat"
    symlink_settings_dir.mkdir(parents=True, mode=0o700)
    symlink_target = tmp_path / "must-not-change"
    symlink_target.write_text("sentinel\n", encoding="utf-8")
    (symlink_settings_dir / "MemoChatQml.conf").symlink_to(symlink_target)
    symlink_env = os.environ.copy()
    symlink_env["XDG_CONFIG_HOME"] = str(symlink_config_home)
    run([str(probe)], env=symlink_env)
    assert symlink_target.read_text(encoding="utf-8") == "sentinel\n"


def test_release_build_does_not_compile_developer_paths_into_the_client() -> None:
    cmake = CLIENT_CMAKE.read_text(encoding="utf-8")
    live2d = LIVE2D_CORE.read_text(encoding="utf-8")
    main = CLIENT_MAIN.read_text(encoding="utf-8")

    assert 'MEMOCHAT_QML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"' not in cmake
    assert 'MEMOCHAT_LIVE2D_SDK_ROOT="${MEMOCHAT_LIVE2D_SDK_ROOT_NORMALIZED}"' not in cmake
    assert "/data/third_party/live2d" not in live2d
    assert "MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD" in cmake
    assert "SKIP_BUILD_RPATH" in cmake
    assert RELEASE_POLICY_MARKER in main
    assert "MEMOCHAT_CLIENT_DISTRIBUTABLE_BUILD=$<BOOL:" in cmake
    assert "ValidateClientReleaseResources.cmake" in cmake


def test_release_resource_validator_rejects_local_live2d_assets(tmp_path: Path) -> None:
    safe_manifest = tmp_path / "safe.qrc"
    safe_manifest.write_text(
        "<RCC><qresource><file>features/pet/view/Live2DCharacterPane.qml</file></qresource></RCC>\n",
        encoding="utf-8",
    )
    safe_probe = tmp_path / "validate-safe.cmake"
    safe_probe.write_text(
        textwrap.dedent(
            f"""
            include("{RELEASE_RESOURCE_VALIDATOR}")
            memochat_validate_client_release_resources(
                CLIENT_ROOT "{tmp_path}"
                MANIFESTS "{safe_manifest}"
            )
            """
        ),
        encoding="utf-8",
    )
    run(["cmake", "-P", str(safe_probe)])

    manifest = tmp_path / "restricted.qrc"
    manifest.write_text(
        "<RCC><qresource><file>../live2d/KafuuChino/model.moc3</file></qresource></RCC>\n",
        encoding="utf-8",
    )
    probe = tmp_path / "validate.cmake"
    probe.write_text(
        textwrap.dedent(
            f"""
            include("{RELEASE_RESOURCE_VALIDATOR}")
            memochat_validate_client_release_resources(
                CLIENT_ROOT "{tmp_path}"
                MANIFESTS "{manifest}"
            )
            """
        ),
        encoding="utf-8",
    )

    result = subprocess.run(["cmake", "-P", str(probe)], text=True, capture_output=True)

    assert result.returncode != 0
    assert "restricted" in result.stderr.lower()


def test_linux_client_packager_uses_a_fresh_allowlisted_stage() -> None:
    assert PACKAGE_SCRIPT.is_file()
    script = PACKAGE_SCRIPT.read_text(encoding="utf-8")

    assert "mktemp -d" in script
    assert "patchelf" in script
    assert "ldd" in script
    assert "sha256sum" in script
    assert "MANIFEST.sha256" in script
    assert "libLive2DCubismCore" in script
    assert "BEGIN PRIVATE KEY" in script
    assert "not found" in script
    assert "tar" in script
    assert "qmlimportscanner" in script
    assert "jq" in script
    assert 'cp -a "$qt_root/qml/."' not in script
    assert 'cp -a "$qt_root/plugins/."' not in script
    assert "sqldrivers/libqsqlite.so" in script
    assert "libqsqlmimer.so" not in script
    assert "platformthemes/libqgtk3.so" not in script
    assert '"$base" == libicu*.so*' not in script
    assert "--library-dir" in script
    assert 'dependency_search_path="${dependency_search_path}${LD_LIBRARY_PATH' not in script
    assert "clean_ldd" in script
    assert "/usr/bin/env -i" in script
    assert "PATH=/usr/bin:/bin" in script
    assert "/usr/bin/ldd" in script
    assert "env -u LD_LIBRARY_PATH" not in script
    assert "MEMOCHAT_RELEASE_POLICY:v1" in script
    assert "configparser" in script
    assert "verify_release_tree.sh" in script
    assert "client_release_scan.allowlist" in script
    assert "is_project_owned_release_file" in script
    assert "verify_release_legal.sh" in script
    assert 'legal_args=(--project-root "$PROJECT_ROOT")' in script
    assert 'legal_args+=(--source-sha "$source_sha")' in script
    assert '"$LEGAL_VERIFIER" "${legal_args[@]}" --copy-to "$stage/legal"' in script
    assert "--ca-cert" in script
    assert "openssl x509" in script
    verifier = RELEASE_TREE_VERIFIER.read_text(encoding="utf-8")
    assert "/usr/bin/env -i" in verifier
    assert "PATH=/usr/bin:/bin" in verifier
    assert "/usr/bin/ldd" in verifier


def make_fake_qt_root(tmp_path: Path) -> Path:
    qt_root = tmp_path / "qt"
    for directory in ("lib", "plugins", "qml"):
        (qt_root / directory).mkdir(parents=True, exist_ok=True)
    return qt_root


def package_probe(
    binary: Path,
    config: Path,
    qt_root: Path,
    output: Path,
    *,
    ca_cert: Path | None = None,
    env: dict[str, str] | None = None,
    library_dirs: tuple[Path, ...] = (),
) -> subprocess.CompletedProcess[str]:
    command = [
        str(PACKAGE_SCRIPT),
        "--binary",
        str(binary),
        "--config",
        str(config),
        "--qt-root",
        str(qt_root),
        "--output-dir",
        str(output),
    ]
    if ca_cert is not None:
        command.extend(("--ca-cert", str(ca_cert)))
    for library_dir in library_dirs:
        command.extend(("--library-dir", str(library_dir)))
    return subprocess.run(
        command,
        text=True,
        capture_output=True,
        env=env,
    )


def create_self_signed_certificate(tmp_path: Path) -> tuple[Path, Path]:
    openssl = shutil.which("openssl")
    if not openssl:
        pytest.skip("openssl is required for the local CA package probe")

    certificate = tmp_path / "local-ca.crt"
    private_key = tmp_path / "local-ca.key"
    run(
        [
            openssl,
            "req",
            "-x509",
            "-newkey",
            "rsa:2048",
            "-nodes",
            "-days",
            "2",
            "-subj",
            "/CN=MemoChat local release test",
            "-keyout",
            str(private_key),
            "-out",
            str(certificate),
        ]
    )
    return certificate, private_key


def test_linux_client_packager_embeds_only_a_valid_public_ca(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the local CA package probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[GateServer]\nca_file=\n[Log]\nRedact=true\n", encoding="utf-8")
    certificate, private_key = create_self_signed_certificate(tmp_path)
    output = tmp_path / "out"

    result = package_probe(
        binary,
        config,
        make_fake_qt_root(tmp_path),
        output,
        ca_cert=certificate,
    )

    assert result.returncode == 0, result.stderr
    portable_dir = output / "MemoChatQml-linux-x86_64"
    packaged_certificate = portable_dir / "certs/memochat-local-ca.crt"
    assert packaged_certificate.read_bytes() == certificate.read_bytes()
    assert stat.S_IMODE(packaged_certificate.stat().st_mode) == 0o644
    packaged_config = configparser.ConfigParser(interpolation=None)
    packaged_config.read(portable_dir / "config.ini", encoding="utf-8")
    assert packaged_config["GateServer"]["ca_file"] == "certs/memochat-local-ca.crt"
    release_info = (portable_dir / "RELEASE-INFO.txt").read_text(encoding="utf-8")
    assert "Deployment trust anchor: ./certs/memochat-local-ca.crt" in release_info
    assert private_key.name not in {path.name for path in portable_dir.rglob("*")}


def test_linux_client_packager_does_not_embed_a_ca_by_default(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the default trust-store package probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[GateServer]\nca_file=\n[Log]\nRedact=true\n", encoding="utf-8")
    output = tmp_path / "out"

    result = package_probe(binary, config, make_fake_qt_root(tmp_path), output)

    assert result.returncode == 0, result.stderr
    portable_dir = output / "MemoChatQml-linux-x86_64"
    assert not (portable_dir / "certs").exists()
    packaged_config = configparser.ConfigParser(interpolation=None)
    packaged_config.read(portable_dir / "config.ini", encoding="utf-8")
    assert packaged_config["GateServer"]["ca_file"] == ""
    release_info = (portable_dir / "RELEASE-INFO.txt").read_text(encoding="utf-8")
    assert "Deployment trust anchor: system trust store" in release_info


@pytest.mark.parametrize("certificate_kind", ("invalid", "with-private-key", "with-extra-text"))
def test_linux_client_packager_rejects_an_invalid_or_private_ca_file(
    tmp_path: Path,
    certificate_kind: str,
) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the invalid CA package probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[GateServer]\nca_file=\n[Log]\nRedact=true\n", encoding="utf-8")
    if certificate_kind == "invalid":
        certificate = tmp_path / "not-a-certificate.crt"
        certificate.write_text("not an X.509 certificate\n", encoding="utf-8")
    elif certificate_kind == "with-private-key":
        public_certificate, private_key = create_self_signed_certificate(tmp_path)
        certificate = tmp_path / "certificate-with-private-key.pem"
        certificate.write_bytes(public_certificate.read_bytes() + private_key.read_bytes())
    else:
        public_certificate, _ = create_self_signed_certificate(tmp_path)
        certificate = tmp_path / "certificate-with-extra-text.pem"
        certificate.write_bytes(public_certificate.read_bytes() + b"password=must-not-enter-release\n")

    result = package_probe(
        binary,
        config,
        make_fake_qt_root(tmp_path),
        tmp_path / "out",
        ca_cert=certificate,
    )

    assert result.returncode != 0
    assert "certificate" in result.stderr.lower() or "private key" in result.stderr.lower()
    assert not (tmp_path / "out/MemoChatQml-linux-x86_64").exists()


def test_linux_client_packager_rejects_a_preconfigured_ca_without_ca_input(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the preconfigured CA package probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text(
        "[GateServer]\nca_file=/etc/memochat/local-ca.crt\n[Log]\nRedact=true\n",
        encoding="utf-8",
    )

    result = package_probe(binary, config, make_fake_qt_root(tmp_path), tmp_path / "out")

    assert result.returncode != 0
    assert "--ca-cert" in result.stderr
    assert not (tmp_path / "out/MemoChatQml-linux-x86_64").exists()


@pytest.mark.parametrize(
    "config_text",
    [
        "[Log]\nRedact=\n",
        "[Log]\nRedact=unknown\n",
        "[Log]\nRedact=true\nRedact=\n",
        "[Log]\nRedact=true\n[Account]\nemail=registered@example.invalid\n",
        "[Log]\nRedact=true\n[AI]\nBaseUrl=https://api.invalid?access_token=release-secret\n",
        "[Log]\nRedact=true\n[Cache]\nRoot=/root/code/MemoChat/private\n",
    ],
)
def test_linux_client_packager_rejects_unsafe_public_config(tmp_path: Path, config_text: str) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the public config probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text(config_text, encoding="utf-8")

    result = package_probe(binary, config, make_fake_qt_root(tmp_path), tmp_path / "out")

    assert result.returncode != 0
    assert "public" in result.stderr


def test_linux_client_packager_does_not_inherit_ambient_library_path(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the dependency closure probe")

    library_source = tmp_path / "ambient.c"
    library_source.write_text("int ambient_probe(void) { return 7; }\n", encoding="utf-8")
    library = tmp_path / "libambientprobe.so"
    run(
        [
            compiler,
            "-shared",
            "-fPIC",
            "-Wl,-soname,libambientprobe.so",
            str(library_source),
            "-o",
            str(library),
        ]
    )
    source = tmp_path / "main.c"
    source.write_text(
        release_probe_source("int ambient_probe(void); int main(void) { return ambient_probe(); }"),
        encoding="utf-8",
    )
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), f"-L{tmp_path}", "-lambientprobe", "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")
    environment = {**os.environ, "LD_LIBRARY_PATH": str(tmp_path)}

    result = package_probe(
        binary,
        config,
        make_fake_qt_root(tmp_path),
        tmp_path / "out",
        env=environment,
    )

    assert result.returncode != 0
    assert not (tmp_path / "out/MemoChatQml-linux-x86_64/lib/libambientprobe.so").exists()


def test_linux_client_packager_uses_the_system_ldd_in_a_clean_environment(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the clean ldd probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")

    fake_bin = tmp_path / "fake-bin"
    fake_bin.mkdir()
    canary = tmp_path / "ambient-ldd-executed"
    fake_ldd = fake_bin / "ldd"
    fake_ldd.write_text(
        f'#!/bin/sh\nprintf \'%s\\n\' "$*" >> {canary}\nexec /usr/bin/ldd "$@"\n',
        encoding="utf-8",
    )
    fake_ldd.chmod(0o755)
    environment = {**os.environ, "PATH": f"{fake_bin}:{os.environ['PATH']}"}

    result = package_probe(
        binary,
        config,
        make_fake_qt_root(tmp_path),
        tmp_path / "out",
        env=environment,
    )

    assert result.returncode == 0, result.stderr
    assert not canary.exists(), canary.read_text(encoding="utf-8")
    release_info = (tmp_path / "out/MemoChatQml-linux-x86_64/RELEASE-INFO.txt").read_text(encoding="utf-8")
    assert "Minimum glibc: GLIBC_" in release_info


def test_linux_client_packager_rejects_input_runpath_dependency(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the input RUNPATH probe")

    hidden_dir = tmp_path / "hidden"
    hidden_dir.mkdir()
    library_source = tmp_path / "hidden.c"
    library_source.write_text("int hidden_probe(void) { return 11; }\n", encoding="utf-8")
    library = hidden_dir / "libhiddenprobe.so"
    run(
        [
            compiler,
            "-shared",
            "-fPIC",
            "-Wl,-soname,libhiddenprobe.so",
            str(library_source),
            "-o",
            str(library),
        ]
    )
    source = tmp_path / "main.c"
    source.write_text(
        release_probe_source("int hidden_probe(void); int main(void) { return hidden_probe(); }"),
        encoding="utf-8",
    )
    binary = tmp_path / "MemoChatQml"
    run(
        [
            compiler,
            str(source),
            f"-L{hidden_dir}",
            f"-Wl,-rpath,{hidden_dir}",
            "-lhiddenprobe",
            "-o",
            str(binary),
        ]
    )
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")

    result = package_probe(binary, config, make_fake_qt_root(tmp_path), tmp_path / "out")

    assert result.returncode != 0
    assert "RPATH" in result.stderr or "RUNPATH" in result.stderr
    assert not (tmp_path / "out/MemoChatQml-linux-x86_64/lib/libhiddenprobe.so").exists()


def test_linux_client_packager_accepts_packaged_dependencies_under_workspace_output(
    tmp_path: Path,
) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the workspace output dependency probe")

    runtime_dir = tmp_path / "runtime"
    runtime_dir.mkdir()
    library_source = tmp_path / "workspace_dependency.c"
    library_source.write_text("int workspace_dependency(void) { return 17; }\n", encoding="utf-8")
    library = runtime_dir / "libworkspace_dependency.so"
    run(
        [
            compiler,
            "-shared",
            "-fPIC",
            "-Wl,-soname,libworkspace_dependency.so",
            str(library_source),
            "-o",
            str(library),
        ]
    )
    source = tmp_path / "main.c"
    source.write_text(
        release_probe_source("int workspace_dependency(void); int main(void) { return workspace_dependency(); }"),
        encoding="utf-8",
    )
    binary = tmp_path / "MemoChatQml"
    run(
        [
            compiler,
            str(source),
            f"-L{runtime_dir}",
            "-lworkspace_dependency",
            "-o",
            str(binary),
        ]
    )
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")

    with tempfile.TemporaryDirectory(prefix=".client-package-test-", dir=REPO_ROOT) as workspace_temp:
        output = Path(workspace_temp) / "out"
        result = package_probe(
            binary,
            config,
            make_fake_qt_root(tmp_path),
            output,
            library_dirs=(runtime_dir,),
        )

        assert result.returncode == 0, result.stderr
        packaged_library = output / "MemoChatQml-linux-x86_64/lib/libworkspace_dependency.so"
        assert packaged_library.is_file()


def test_linux_client_packager_rejects_dependency_resolved_outside_package(
    tmp_path: Path,
) -> None:
    compiler = shutil.which("cc")
    patchelf = shutil.which("patchelf")
    if not compiler or not patchelf:
        pytest.skip("a C compiler and patchelf are required for the dependency containment probe")

    runtime_dir = tmp_path / "runtime"
    runtime_dir.mkdir()
    library_source = tmp_path / "outside_dependency.c"
    library_source.write_text("int outside_dependency(void) { return 23; }\n", encoding="utf-8")
    library = runtime_dir / "liboutside_dependency.so"
    run(
        [
            compiler,
            "-shared",
            "-fPIC",
            "-Wl,-soname,liboutside_dependency.so",
            str(library_source),
            "-o",
            str(library),
        ]
    )
    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    run([patchelf, "--add-needed", str(library), str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")

    result = package_probe(
        binary,
        config,
        make_fake_qt_root(tmp_path),
        tmp_path / "out",
        library_dirs=(runtime_dir,),
    )

    assert result.returncode != 0
    assert "outside the package" in result.stderr


def test_linux_client_packager_rejects_binary_without_release_policy_marker(
    tmp_path: Path,
) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the release policy probe")

    source = tmp_path / "main.c"
    source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")

    result = package_probe(binary, config, make_fake_qt_root(tmp_path), tmp_path / "out")

    assert result.returncode != 0
    assert "release policy marker" in result.stderr


def test_linux_client_packager_rejects_restricted_asset_marker(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the restricted asset probe")

    source = tmp_path / "main.c"
    source.write_text(
        release_probe_source('static const char asset[] = "KafuuChino"; int main(void) { return asset[0] == 0; }'),
        encoding="utf-8",
    )
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")

    result = package_probe(binary, config, make_fake_qt_root(tmp_path), tmp_path / "out")

    assert result.returncode != 0
    assert "restricted" in result.stderr.lower()


def test_linux_client_packager_rejects_email_in_staged_binary(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the staged email probe")

    source = tmp_path / "main.c"
    source.write_text(
        release_probe_source(
            'static const char account[] = "registered@example.invalid"; int main(void) { return account[0] == 0; }'
        ),
        encoding="utf-8",
    )
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")

    result = package_probe(binary, config, make_fake_qt_root(tmp_path), tmp_path / "out")

    assert result.returncode != 0
    assert "email" in result.stderr.lower()


def test_linux_client_packager_rejects_native_live2d_binary(tmp_path: Path) -> None:
    if not PACKAGE_SCRIPT.is_file():
        pytest.fail("Linux client package script is missing")

    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the native Live2D package probe")

    live2d_source = tmp_path / "live2d.c"
    live2d_source.write_text("int live2d_probe(void) { return 0; }\n", encoding="utf-8")
    live2d_library = tmp_path / "libLive2DCubismCore.so"
    run(
        [
            compiler,
            "-shared",
            "-fPIC",
            "-Wl,-soname,libLive2DCubismCore.so",
            str(live2d_source),
            "-o",
            str(live2d_library),
        ]
    )
    binary_source = tmp_path / "main.c"
    binary_source.write_text(
        release_probe_source("int live2d_probe(void); int main(void) { return live2d_probe(); }"),
        encoding="utf-8",
    )
    fake_binary = tmp_path / "MemoChatQml"
    run(
        [
            compiler,
            str(binary_source),
            f"-L{tmp_path}",
            f"-Wl,-rpath,{tmp_path}",
            "-lLive2DCubismCore",
            "-o",
            str(fake_binary),
        ]
    )
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")
    output = tmp_path / "out"

    result = subprocess.run(
        [
            str(PACKAGE_SCRIPT),
            "--binary",
            str(fake_binary),
            "--config",
            str(config),
            "--output-dir",
            str(output),
        ],
        text=True,
        capture_output=True,
        env={**os.environ, "LC_ALL": "C"},
    )

    assert result.returncode != 0
    assert "Live2D" in result.stderr


def test_linux_client_packager_rejects_disabled_log_redaction(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the public config package probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=false\n", encoding="utf-8")
    qt_root = tmp_path / "qt"
    for directory in ("lib", "plugins", "qml"):
        (qt_root / directory).mkdir(parents=True, exist_ok=True)

    result = subprocess.run(
        [
            str(PACKAGE_SCRIPT),
            "--binary",
            str(binary),
            "--config",
            str(config),
            "--qt-root",
            str(qt_root),
            "--output-dir",
            str(tmp_path / "out"),
        ],
        text=True,
        capture_output=True,
    )

    assert result.returncode != 0
    assert "Redact" in result.stderr


def test_linux_client_packager_creates_audited_portable_outputs(tmp_path: Path) -> None:
    compiler = shutil.which("cc")
    if not compiler:
        pytest.skip("a C compiler is required for the package integration probe")

    source = tmp_path / "main.c"
    source.write_text(release_probe_source("int main(void) { return 0; }"), encoding="utf-8")
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")
    qt_root = tmp_path / "qt"
    for directory in ("lib", "plugins", "qml"):
        (qt_root / directory).mkdir(parents=True, exist_ok=True)
    sentinel = tmp_path / "registration-email.txt"
    sentinel.write_text("user@example.invalid\n", encoding="utf-8")
    output = tmp_path / "out"
    artifact_name = "MemoChatQml-test-linux-x86_64"

    run(
        [
            str(PACKAGE_SCRIPT),
            "--binary",
            str(binary),
            "--config",
            str(config),
            "--qt-root",
            str(qt_root),
            "--output-dir",
            str(output),
            "--artifact-name",
            artifact_name,
        ]
    )

    portable_dir = output / artifact_name
    archive = output / f"{artifact_name}.tar.gz"
    assert (portable_dir / "MemoChat").is_file()
    assert (portable_dir / "MemoChatQml").is_file()
    assert (portable_dir / "config.ini").is_file()
    assert (portable_dir / "legal/LICENSE").read_text(encoding="utf-8") == (REPO_ROOT / "LICENSE").read_text(
        encoding="utf-8"
    )
    assert (portable_dir / "legal/THIRD_PARTY_NOTICES.md").read_text(encoding="utf-8") == (
        REPO_ROOT / "THIRD_PARTY_NOTICES.md"
    ).read_text(encoding="utf-8")
    legal_status = (portable_dir / "legal/LEGAL-STATUS.txt").read_text(encoding="utf-8")
    assert "third_party_legal_corpus=complete" in legal_status
    assert "formal_distribution_ready=false" in legal_status
    assert (portable_dir / "legal/third-party").is_dir()
    release_info = (portable_dir / "RELEASE-INFO.txt").read_text(encoding="utf-8")
    assert "Legal inventory: complete" in release_info
    assert "Third-party legal corpus: complete" in release_info
    assert "Formal distribution ready: false" in release_info
    assert (portable_dir / "MANIFEST.sha256").is_file()
    assert archive.is_file()
    assert Path(f"{archive}.sha256").is_file()
    assert not (portable_dir / sentinel.name).exists()
    run(["sha256sum", "--check", "MANIFEST.sha256"], cwd=portable_dir)
    run(["sha256sum", "--check", Path(f"{archive}.sha256").name], cwd=output)
    assert "not stripped" not in run(["file", str(portable_dir / "MemoChatQml")]).stdout
    dynamic = run(["readelf", "-d", str(portable_dir / "MemoChatQml")]).stdout
    assert "$ORIGIN/lib" in dynamic
    release_policy = run(["readelf", "-p", ".note.memochat.release", str(portable_dir / "MemoChatQml")]).stdout
    assert RELEASE_POLICY_MARKER in release_policy


def test_linux_client_packager_bundles_compiler_runtime_but_not_glibc(tmp_path: Path) -> None:
    compiler = shutil.which("g++")
    if not compiler:
        pytest.skip("g++ is required for the compiler runtime package probe")

    source = tmp_path / "main.cpp"
    source.write_text(
        release_probe_source('#include <iostream>\nint main() { std::cout << "MemoChat"; return 0; }'),
        encoding="utf-8",
    )
    binary = tmp_path / "MemoChatQml"
    run([compiler, str(source), "-o", str(binary)])
    config = tmp_path / "config.ini"
    config.write_text("[Log]\nRedact=true\n", encoding="utf-8")
    qt_root = tmp_path / "qt"
    for directory in ("lib", "plugins", "qml"):
        (qt_root / directory).mkdir(parents=True, exist_ok=True)
    output = tmp_path / "out"
    artifact_name = "MemoChatQml-compiler-runtime-test"

    run(
        [
            str(PACKAGE_SCRIPT),
            "--binary",
            str(binary),
            "--config",
            str(config),
            "--qt-root",
            str(qt_root),
            "--output-dir",
            str(output),
            "--artifact-name",
            artifact_name,
        ]
    )

    bundled_libraries = {path.name for path in (output / artifact_name / "lib").iterdir()}
    assert "libstdc++.so.6" in bundled_libraries
    assert "libgcc_s.so.1" in bundled_libraries
    assert "libc.so.6" not in bundled_libraries
    compiler_rpath = run(["readelf", "-d", str(output / artifact_name / "lib/libstdc++.so.6")]).stdout
    assert "[$ORIGIN]" in compiler_rpath
    assert "[\\$ORIGIN]" not in compiler_rpath
    script = PACKAGE_SCRIPT.read_text(encoding="utf-8")
    assert "libatomic.so.1" in script
