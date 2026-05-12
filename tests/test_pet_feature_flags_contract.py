import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
ROOT_CMAKE = REPO_ROOT / "CMakeLists.txt"
AI_CONFIG = REPO_ROOT / "apps/server/core/AIOrchestrator/config.py"
AI_CONFIG_YAML = REPO_ROOT / "apps/server/core/AIOrchestrator/config.yaml"
PET_ROUTER = REPO_ROOT / "apps/server/core/AIOrchestrator/api/pet_router.py"
PET_RUNTIME = REPO_ROOT / "apps/server/core/AIOrchestrator/harness/pet/runtime.py"
AI_DOCKER_COMPOSE = REPO_ROOT / "apps/server/core/AIOrchestrator/docker-compose.yml"
CLIENT_CMAKE = REPO_ROOT / "apps/client/desktop/MemoChat-qml/CMakeLists.txt"
LIVE2D_FIND_MODULE = REPO_ROOT / "cmake/FindLive2DCubism.cmake"
LIVE2D_POLICY_DOC = REPO_ROOT / "apps/client/desktop/MemoChat-qml/docs/live2d-desktop-pet-assets.md"


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
            "voice_clone_enabled: bool = False",
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
            "voice_clone_enabled: false",
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
            "MEMOCHAT_PET_VOICE_CLONE",
        ):
            self.assertIn(token, config)

        self.assertIn("_merge_pet_alias_env", config)
        self.assertIn("_PET_ENV_ALIASES", config)

        for token in (
            "MEMOCHAT_ENABLE_PET=${MEMOCHAT_ENABLE_PET:-true}",
            "MEMOCHAT_PET_DETERMINISTIC=${MEMOCHAT_PET_DETERMINISTIC:-true}",
            "MEMOCHAT_ENABLE_LIVE2D_NATIVE=${MEMOCHAT_ENABLE_LIVE2D_NATIVE:-false}",
            "MEMOCHAT_PET_CLOUD_VISION=${MEMOCHAT_PET_CLOUD_VISION:-false}",
            "MEMOCHAT_PET_VOICE_CLONE=${MEMOCHAT_PET_VOICE_CLONE:-false}",
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

    def test_client_cmake_exposes_live2d_native_placeholders(self):
        cmake = read(CLIENT_CMAKE)

        self.assertIn("option(MEMOCHAT_ENABLE_LIVE2D_NATIVE", cmake)
        self.assertIn("set(MEMOCHAT_LIVE2D_SDK_ROOT", cmake)
        self.assertIn("if(MEMOCHAT_ENABLE_LIVE2D_NATIVE)", cmake)
        self.assertIn("find_package(Live2DCubism REQUIRED)", cmake)
        self.assertIn("Live2DCubism::Native", cmake)
        self.assertIn("using MemoChat placeholder pet renderer", cmake)
        self.assertIn("target_compile_definitions(MemoChatQml PRIVATE", cmake)
        self.assertIn("MEMOCHAT_ENABLE_LIVE2D_NATIVE=$<BOOL:${MEMOCHAT_ENABLE_LIVE2D_NATIVE}>", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_SDK_ROOT_NORMALIZED", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_SDK_ROOT", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_CUBISM_FOUND=$<BOOL:${MEMOCHAT_LIVE2D_NATIVE_AVAILABLE}>", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_CORE_INCLUDE_DIR", cmake)
        self.assertIn("MEMOCHAT_LIVE2D_FRAMEWORK_INCLUDE_DIR", cmake)

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
            "placeholder renderer",
        ):
            self.assertIn(token, doc)


if __name__ == "__main__":
    unittest.main()
