import json
import unittest
from pathlib import Path

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
ROOT_CMAKE = REPO_ROOT / "CMakeLists.txt"
CMAKE_PRESETS = REPO_ROOT / "CMakePresets.json"
AI_CONFIG = REPO_ROOT / "apps/server/core/AIOrchestrator/config.py"
AI_CONFIG_YAML = REPO_ROOT / "apps/server/core/AIOrchestrator/config.yaml"
PET_ROUTER = REPO_ROOT / "apps/server/core/AIOrchestrator/api/pet_router.py"
PET_RUNTIME = REPO_ROOT / "apps/server/core/AIOrchestrator/harness/pet/runtime.py"
AI_DOCKER_COMPOSE = REPO_ROOT / "apps/server/core/AIOrchestrator/docker-compose.yml"
CLIENT_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/CMakeLists.txt"
LIVE2D_SOURCES_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/live2d/sources.cmake"
LIVE2D_RENDER_ITEM_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/live2d/rendering/Live2DRenderItem.cpp"
LIVE2D_OFFICIAL_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/live2d/rendering/Live2DOfficialOpenGLRenderer.cpp"
LIVE2D_PLACEHOLDER_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/live2d/rendering/Live2DPlaceholderRenderer.cpp"
LIVE2D_FIND_MODULE = REPO_ROOT / "cmake/FindLive2DCubism.cmake"
LIVE2D_POLICY_DOC = REPO_ROOT / "apps/client/desktop/MemoChat-qml/docs/live2d-desktop-pet-assets.md"
WSLG_LAUNCHER = REPO_ROOT / "tools/scripts/status/start-memochat-qml-wslg.sh"


def read(path):
    return path.read_text(encoding="utf-8")


class PetFeatureFlagsContractTests(unittest.TestCase):
    def test_ai_orchestrator_declares_pet_feature_config_and_defaults(self):
        config = read(AI_CONFIG)
        yaml = read(AI_CONFIG_YAML)

        self.assertIn("class PetFeatureConfig", config)
        self.assertIn("pet: PetFeatureConfig", config)
        for token in (
            "enabled: bool = True",
            "deterministic: bool = True",
            "live2d_native_enabled: bool = False",
            'live2d_sdk_root: str = ""',
            'asset_root: str = ""',
            "cloud_vision_enabled: bool = False",
            "local_vision_enabled: bool = False",
            "vision_camera_index: int = 0",
            'vision_analyzer: str = "opencv"',
            "vision_retain_raw_frames: bool = False",
            "voice_clone_enabled: bool = False",
            'voice_provider: str = "scripted"',
            'voice_sovits_base_url: str = ""',
            'voice_sovits_reference_audio: str = ""',
            'voice_sovits_output_dir: str = "/app/.data/pet-voice-cache"',
            "voice_training_enabled: bool = True",
            'voice_training_artifact_root: str = "/app/.data/pet-voice-training"',
        ):
            self.assertIn(token, config)

        for token in (
            "pet:",
            "enabled: true",
            "deterministic: true",
            "live2d_native_enabled: false",
            'live2d_sdk_root: ""',
            'asset_root: ""',
            "cloud_vision_enabled: false",
            "local_vision_enabled: false",
            "vision_camera_index: 0",
            'vision_analyzer: "opencv"',
            "vision_retain_raw_frames: false",
            "voice_clone_enabled: false",
            'voice_provider: "scripted"',
            'voice_sovits_base_url: ""',
            'voice_sovits_reference_audio: ""',
            'voice_sovits_output_dir: "/app/.data/pet-voice-cache"',
            "voice_training_enabled: true",
            'voice_training_artifact_root: "/app/.data/pet-voice-training"',
        ):
            self.assertIn(token, yaml)

    def test_global_pet_env_aliases_are_supported(self):
        config = read(AI_CONFIG)
        compose = read(AI_DOCKER_COMPOSE)

        for token in (
            "MEMOCHAT_ENABLE_PET",
            "MEMOCHAT_PET_DETERMINISTIC",
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE",
            "MEMOCHAT_LIVE2D_SDK_ROOT",
            "MEMOCHAT_PET_ASSET_ROOT",
            "MEMOCHAT_PET_CLOUD_VISION",
            "MEMOCHAT_PET_LOCAL_VISION",
            "MEMOCHAT_PET_VISION_CAMERA_INDEX",
            "MEMOCHAT_PET_VISION_ANALYZER",
            "MEMOCHAT_PET_VISION_RETAIN_RAW_FRAMES",
            "MEMOCHAT_PET_VOICE_CLONE",
            "MEMOCHAT_PET_VOICE_PROVIDER",
            "MEMOCHAT_PET_SOVITS_BASE_URL",
            "MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO",
            "MEMOCHAT_PET_SOVITS_OUTPUT_DIR",
            "MEMOCHAT_PET_VOICE_TRAINING",
            "MEMOCHAT_PET_VOICE_TRAINING_ARTIFACT_ROOT",
        ):
            self.assertIn(token, config)

        self.assertIn("_merge_pet_alias_env", config)
        self.assertIn("_PET_ENV_ALIASES", config)

        for token in (
            "MEMOCHAT_ENABLE_PET=${MEMOCHAT_ENABLE_PET:-true}",
            "MEMOCHAT_PET_DETERMINISTIC=${MEMOCHAT_PET_DETERMINISTIC:-true}",
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE=${MEMOCHAT_ENABLE_LIVE2D_NATIVE:-false}",
            "MEMOCHAT_PET_CLOUD_VISION=${MEMOCHAT_PET_CLOUD_VISION:-false}",
            "MEMOCHAT_PET_LOCAL_VISION=${MEMOCHAT_PET_LOCAL_VISION:-false}",
            "MEMOCHAT_PET_VISION_ANALYZER=${MEMOCHAT_PET_VISION_ANALYZER:-opencv}",
            "MEMOCHAT_PET_VOICE_CLONE=${MEMOCHAT_PET_VOICE_CLONE:-false}",
            "MEMOCHAT_PET_VOICE_PROVIDER=${MEMOCHAT_PET_VOICE_PROVIDER:-gpt-sovits}",
            "MEMOCHAT_PET_SOVITS_BASE_URL=${MEMOCHAT_PET_SOVITS_BASE_URL:-http://host.docker.internal:9880}",
            "MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO=${MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO:-/data/gpt-sovits/refs/kafuu-chino-ref.wav}",
            "MEMOCHAT_PET_SOVITS_HOST_DATA_DIR",
            "target: /data/gpt-sovits",
            "MEMOCHAT_PET_SOVITS_OUTPUT_DIR=${MEMOCHAT_PET_SOVITS_OUTPUT_DIR:-/app/.data/pet-voice-cache}",
            "MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE=${MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE:-ja}",
            "MEMOCHAT_PET_VOICE_TRAINING=${MEMOCHAT_PET_VOICE_TRAINING:-true}",
            "MEMOCHAT_PET_VOICE_TRAINING_ARTIFACT_ROOT=${MEMOCHAT_PET_VOICE_TRAINING_ARTIFACT_ROOT:-/app/.data/pet-voice-training}",
        ):
            self.assertIn(token, compose)

    def test_runtime_and_router_respect_feature_flags(self):
        runtime = read(PET_RUNTIME)
        router = read(PET_ROUTER)

        self.assertIn("class PetRuntimeConfig", runtime)
        self.assertIn("self._config", runtime)
        self.assertIn("desktop pet is disabled by configuration", runtime)
        self.assertIn("no provider adapter is configured", runtime)
        self.assertIn("PetRuntime(settings.pet)", router)
        self.assertIn("if not settings.pet.enabled", router)
        self.assertIn("except RuntimeError", router)
        self.assertIn("desktop pet is disabled by configuration", router)
        self.assertIn("create_voice_training_job", runtime)
        self.assertIn("latest_ready_reference_audio", runtime)
        self.assertIn("_configured_or_trained_reference_audio", runtime)
        self.assertIn("VoiceTrainingRequest", router)
        self.assertIn("/voice-training/jobs", router)
        self.assertIn("capture_observation", runtime)
        self.assertIn("VisionCaptureRequest", router)
        self.assertIn("/capture", router)
        self.assertIn("audio_file_path", runtime)
        self.assertIn("/audio/{file_name}", router)

    def test_client_cmake_exposes_live2d_native_error_diagnostics(self):
        cmake = read(CLIENT_CMAKE) + "\n" + read(LIVE2D_SOURCES_CMAKE)
        render_item = read(LIVE2D_RENDER_ITEM_CPP)
        official = read(LIVE2D_OFFICIAL_CPP)
        placeholder = read(LIVE2D_PLACEHOLDER_CPP)

        self.assertIn("option(MEMOCHAT_ENABLE_LIVE2D_NATIVE", cmake)
        self.assertIn("set(MEMOCHAT_LIVE2D_SDK_ROOT", cmake)
        self.assertIn("if(MEMOCHAT_ENABLE_LIVE2D_NATIVE)", cmake)
        self.assertIn("find_package(Live2DCubism REQUIRED)", cmake)
        self.assertIn("Live2DCubism::Native", cmake)
        self.assertIn("Live2DRenderItem will show model error diagnostics", cmake)
        self.assertIn("target_compile_definitions(MemoChatQml PRIVATE", cmake)
        self.assertIn("MEMOCHAT_ENABLE_LIVE2D_NATIVE=$<BOOL:${MEMOCHAT_ENABLE_LIVE2D_NATIVE}>", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_SDK_ROOT_NORMALIZED", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_SDK_ROOT", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_CUBISM_FOUND=$<BOOL:${MEMOCHAT_LIVE2D_NATIVE_AVAILABLE}>", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_CORE_INCLUDE_DIR", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_FRAMEWORK_INCLUDE_DIR", cmake)
        self.assertIn("Live2DOfficialOpenGLRenderer.cpp", cmake)
        self.assertIn("MemoChatLive2DCubismFramework", cmake)
        self.assertIn("CubismRenderer_OpenGLES2", official)
        self.assertIn("std::make_unique<Live2DOfficialOpenGLRenderer>", render_item)
        self.assertNotIn("drawModelErrorMarker", render_item)
        self.assertNotIn("QOpenGLPaintDevice", render_item)
        self.assertNotIn("QImage fallback", render_item)
        self.assertIn("renderStatus", render_item)
        self.assertNotIn("Live2DPlaceholderRenderer.h", render_item)
        self.assertIn("drawPlaceholderPet", placeholder)
        self.assertIn("QPainterPath", placeholder)
        self.assertNotIn("texture_00.png", placeholder)
        self.assertNotIn("texture_01.png", placeholder)

    def test_live2d_cmake_discovery_is_opt_in_and_repo_local(self):
        root_cmake = read(ROOT_CMAKE)
        client_cmake = read(CLIENT_CMAKE)
        finder = read(LIVE2D_FIND_MODULE)

        self.assertIn('list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")', root_cmake)
        self.assertIn('list(APPEND CMAKE_MODULE_PATH "${MEMOCHAT_REPO_ROOT}/cmake")', client_cmake)
        self.assertIn("FindPackageHandleStandardArgs", finder)
        self.assertIn("Live2DCubismCore.hpp", finder)
        self.assertIn("CubismFramework.hpp", finder)
        self.assertIn("Live2DCubism::Core", finder)
        self.assertIn("Live2DCubism::Framework", finder)
        self.assertIn("Live2DCubism::Native", finder)
        self.assertIn("ENV{MEMOCHAT_LIVE2D_SDK_ROOT}", finder)
        self.assertIn("Core/lib/linux/x86_64", finder)
        self.assertIn("NO_DEFAULT_PATH", finder)
        self.assertIn("find_package(Live2DCubism REQUIRED)", client_cmake)
        self.assertLess(
            client_cmake.index("if(MEMOCHAT_ENABLE_LIVE2D_NATIVE)"),
            client_cmake.index("find_package(Live2DCubism REQUIRED)"),
            "Live2D discovery should only run inside the explicit Native opt-in branch",
        )

    def test_live2d_sdk_and_asset_policy_is_documented(self):
        doc = read(LIVE2D_POLICY_DOC)

        for token in (
            "/data/third_party/live2d/CubismSdkForNative-current",
            "/data/memochat/pet-assets",
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE=OFF",
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE=ON",
            "must not be committed",
            "Live2D Cubism SDK binaries",
            ".model3.json",
            ".moc3",
            "Generated speech clips",
            "model error diagnostic",
        ):
            self.assertIn(token, doc)

    def test_cmake_presets_keep_only_windows_full_and_linux_full(self):
        presets = read(CMAKE_PRESETS)
        preset_data = json.loads(presets)
        launcher = read(WSLG_LAUNCHER)
        configure_names = {item["name"] for item in preset_data["configurePresets"]}
        build_names = {item["name"] for item in preset_data["buildPresets"]}
        test_names = {item["name"] for item in preset_data["testPresets"]}

        self.assertEqual(configure_names, {"msvc2022-full", "linux-full-gcc16"})
        self.assertEqual(build_names, {"msvc2022-full", "linux-full-gcc16"})
        self.assertEqual(test_names, {"msvc2022-full", "linux-full-gcc16"})

        for token in (
            '"msvc2022-full"',
            '"linux-full-gcc16"',
            '"MEMOCHAT_ENABLE_LIVE2D_NATIVE": "ON"',
            '"MEMOCHAT_LIVE2D_SDK_ROOT": "/data/third_party/live2d/CubismSdkForNative-current"',
            '"${sourceDir}/build-linux-full-gcc16"',
        ):
            self.assertIn(token, presets)

        full_index = launcher.index("${PROJECT_ROOT}/build-linux-full-gcc16/bin/MemoChatQml")
        runtime_index = launcher.index("${PROJECT_ROOT}/infra/Memo_ops/runtime/services/MemoChatQml/MemoChatQml")
        self.assertLess(full_index, runtime_index)
        self.assertIn("[Live2D build flags]", launcher)
        self.assertIn("MEMOCHAT_ENABLE_LIVE2D_NATIVE", launcher)
        self.assertIn("Native OpenGL is not enabled", launcher)
        self.assertIn("require_live2d_native_client", launcher)
        self.assertIn("live2d_native_cache_is_valid", launcher)
        self.assertIn("MemoChatQml build cache is stale or non-native for Live2D", launcher)


if __name__ == "__main__":
    unittest.main()
