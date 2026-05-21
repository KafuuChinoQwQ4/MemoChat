import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DEPLOY_SCRIPT = REPO_ROOT / "tools/scripts/status/deploy_services.sh"
START_SERVICES_SCRIPT = REPO_ROOT / "tools/scripts/status/start-all-services.sh"
START_QML_SCRIPT = REPO_ROOT / "tools/scripts/status/start-memochat-qml-wslg.sh"
RUN_FULL_STACK_SCRIPT = REPO_ROOT / "tools/scripts/status/run-linux-full-stack.sh"
GPT_SOVITS_VOICE_SCRIPT = REPO_ROOT / "tools/scripts/pet/apply_gpt_sovits_voice_wsl.sh"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


class StatusDeployContractTests(unittest.TestCase):
    def test_linux_deploy_prefers_client_build_for_memochat_qml(self):
        source = read(DEPLOY_SCRIPT)

        self.assertIn("CLIENT_BUILD_BIN", source)
        self.assertIn("MEMOCHAT_CLIENT_BUILD_BIN", source)
        self.assertIn("--client-build-bin", source)
        self.assertIn("copy_optional_from_candidates", source)
        self.assertIn("copy_memochat_qml_from_candidates", source)
        self.assertIn("require_live2d_native_memo_chat_qml", source)

        match = re.search(
            r"copy_memochat_qml_from_candidates\s+\"\$\{RUNTIME_DIR\}/MemoChatQml/MemoChatQml\"(?P<body>.*?)\ncopy_optional_from_candidates",
            source,
            flags=re.S,
        )
        self.assertIsNotNone(match, "MemoChatQml deploy should use ordered source candidates")
        body = match.group("body")
        client_index = body.find("${CLIENT_BUILD_BIN}/MemoChatQml")
        server_index = body.find("${BUILD_BIN}/MemoChatQml")
        self.assertGreaterEqual(client_index, 0)
        self.assertGreater(server_index, client_index)

    def test_linux_deploy_prints_client_build_path_for_diagnostics(self):
        source = read(DEPLOY_SCRIPT)

        self.assertIn("CLIENT_BUILD:", source)
        self.assertIn("MEMOCHAT_CLIENT_BUILD_BIN", source)
        self.assertIn('BUILD_BIN="${MEMOCHAT_BUILD_BIN:-${PROJECT_ROOT}/build-linux-full-gcc16/bin}"', source)
        self.assertIn('CLIENT_BUILD_BIN="${PROJECT_ROOT}/build-linux-full-gcc16/bin"', source)

    def test_linux_deploy_refuses_stale_non_native_live2d_qml_builds(self):
        source = read(DEPLOY_SCRIPT)

        for token in (
            "live2d_native_cache_is_valid",
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE",
            "MEMOCHAT_LIVE2D_SDK_ROOT",
            "Refusing to deploy non-native/stale Live2D MemoChatQml build",
            "Live2D Cubism Native OpenGL is not enabled in this build",
        ):
            self.assertIn(token, source)

    def test_wslg_client_startup_sets_ime_defaults_when_available(self):
        source = read(START_QML_SCRIPT)

        for token in (
            "QT_IM_MODULE",
            "GTK_IM_MODULE",
            "XMODIFIERS",
            "fcitx5",
            "ibus-daemon",
            "ibus-daemon -drx",
            "ibus-daemon -drx --replace",
            "ibus engine libpinyin",
            "ibus exit",
            "pkill -x ibus-daemon",
            "MEMOCHAT_RESTART_IBUS",
            "IBUS_ENABLE_SYNC_MODE",
            "GDK_BACKEND=x11",
            "QT_QPA_PLATFORM=xcb",
            "env -u WAYLAND_DISPLAY",
            "unset WAYLAND_DISPLAY",
            "sleep 0.1",
            "libfcitx5platforminputcontextplugin.so",
            "libibusplatforminputcontextplugin.so",
            "QT_VIRTUALKEYBOARD_DESKTOP_DISABLE",
            "qtvirtualkeyboard",
        ):
            self.assertIn(token, source)

        self.assertIn("select_input_method", source)
        self.assertIn("MEMOCHAT_KEEP_QT_IM_MODULE", source)
        self.assertIn('export QT_IM_MODULE="$qt_module"', source)
        self.assertIn("has_qt_fcitx_plugin", source)
        self.assertIn("has_qt_ibus_plugin", source)
        self.assertIn("env -u WAYLAND_DISPLAY fcitx5 -d", source)
        self.assertIn("start_ibus_daemon", source)
        self.assertIn("select_ibus_libpinyin", source)
        self.assertIn("stop_ibus_daemons", source)
        self.assertIn("start_ibus_daemon replace", source)
        self.assertIn("env -u WAYLAND_DISPLAY ibus address", source)
        self.assertIn("ibus engine libpinyin", source)
        self.assertIn("sleep 0.1", source)
        self.assertIn("sleep 0.15", source)
        self.assertIn("IBUS_SYNC_MODE:", source)
        self.assertIn("GDK_BACKEND:", source)
        fcitx_index = source.index("has_qt_fcitx_plugin")
        ibus_index = source.index("elif [[ \"$has_qt_ibus_plugin\" -eq 1 ]]")
        virtual_keyboard_index = source.index('elif [[ -f "${QT_ROOT}/plugins/platforminputcontexts/libqtvirtualkeyboardplugin.so" ]]')
        self.assertLess(fcitx_index, virtual_keyboard_index)
        self.assertLess(ibus_index, virtual_keyboard_index)
        self.assertIn("dbus-launch", source)
        self.assertIn("[input method]", source)

    def test_linux_full_stack_launcher_chains_build_deploy_backend_and_client(self):
        source = read(RUN_FULL_STACK_SCRIPT)

        for token in (
            "source \"$ENV_FILE\"",
            "cmake --preset \"$PRESET\"",
            "cmake --build --preset \"$PRESET\" --parallel \"$BUILD_PARALLEL\"",
            "docker compose -f \"$AI_COMPOSE_FILE\" up -d --build",
            '"${SCRIPT_DIR}/deploy_services.sh" --build-bin "$BUILD_BIN"',
            '"${SCRIPT_DIR}/start-all-services.sh" --no-deploy',
            '"${SCRIPT_DIR}/start-memochat-qml-wslg.sh" --exe "$CLIENT_EXE"',
            'CLIENT_EXE="$(project_path "$CLIENT_EXE")"',
            "linux-full-gcc16",
            "build-linux-full-gcc16",
            "apps/server/core/AIOrchestrator/docker-compose.yml",
            "MEMOCHAT_BUILD_PARALLEL",
            "MEMOCHAT_REQUIRE_GPT_SOVITS",
            "MEMOCHAT_AI_BASE_URL",
            "MEMOCHAT_AI_VOICE_WAIT_SECONDS",
            "ensure_ai_voice_ready",
            "/pet/diagnostics/voice?probe_endpoint=true",
            "--skip-build",
            "--skip-ai-build",
            "--skip-deploy",
            "--skip-backend",
            "--no-client",
            "--client-diagnose",
        ):
            self.assertIn(token, source)

        self.assertIn("set -Eeuo pipefail", source)
        self.assertIn("exec \"${client_args[@]}\"", source)

    def test_linux_start_requires_gpt_sovits_by_default(self):
        source = read(START_SERVICES_SCRIPT)

        for token in (
            "MEMOCHAT_REQUIRE_GPT_SOVITS",
            "REQUIRE_GPT_SOVITS",
            "print_gpt_sovits_failure_hint",
            "GPT-SoVITS did not become ready",
            "smoke_gpt_sovits_tts_wsl.sh",
            "apply_gpt_sovits_voice_wsl.sh",
            "To intentionally allow text-only pet replies",
            "return 1",
            "--skip-gpt-sovits",
        ):
            self.assertIn(token, source)

    def test_gpt_sovits_voice_script_recreates_ai_orchestrator_with_reference_audio(self):
        source = read(GPT_SOVITS_VOICE_SCRIPT)

        for token in (
            "smoke_gpt_sovits_tts_wsl.sh",
            "MEMOCHAT_PET_VOICE_PROVIDER",
            "gpt-sovits",
            "MEMOCHAT_PET_SOVITS_BASE_URL",
            "http://host.docker.internal:9880",
            "MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO",
            "refs/kafuu-chino-ref.wav",
            "MEMOCHAT_PET_SOVITS_PROMPT_TEXT",
            "MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE",
            "MEMOCHAT_PET_SOVITS_TEXT_LANGUAGE",
            "MEMOCHAT_PET_SOVITS_HOST_DATA_DIR",
            "GPT_SOVITS_HOST",
            "0.0.0.0",
            "pkill -f",
            "docker compose",
            "up -d --force-recreate memochat-ai-orchestrator",
            "/pet/diagnostics/voice?probe_endpoint=true",
        ):
            self.assertIn(token, source)

    def test_gpt_sovits_api_launcher_daemonizes_and_waits_for_docs(self):
        source = read(REPO_ROOT / "tools/scripts/pet/start_gpt_sovits_api_wsl.sh")

        for token in (
            "PID_FILE",
            "START_WAIT_SECONDS",
            "--foreground",
            "nohup",
            "wait_for_api",
            "GPT-SoVITS already ready",
            "GPT-SoVITS ready (pid=",
            "GPT-SoVITS failed to become ready",
        ):
            self.assertIn(token, source)

    def test_gpt_sovits_smoke_starts_launcher_directly(self):
        source = read(REPO_ROOT / "tools/scripts/pet/smoke_gpt_sovits_tts_wsl.sh")

        self.assertNotIn("setsid -f \"$START_SCRIPT\"", source)
        self.assertIn("\"$START_SCRIPT\"", source)


if __name__ == "__main__":
    unittest.main()
