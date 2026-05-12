from __future__ import annotations

import os
import unittest
from contextlib import contextmanager

from config import Settings
from harness.pet import PetRuntime, PetRuntimeConfig


@contextmanager
def patched_env(**values: str):
    old_values = {key: os.environ.get(key) for key in values}
    try:
        for key, value in values.items():
            os.environ[key] = value
        yield
    finally:
        for key, old_value in old_values.items():
            if old_value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = old_value


class PetFeatureFlagTests(unittest.IsolatedAsyncioTestCase):
    def test_pet_feature_defaults_are_safe(self):
        settings = Settings()

        self.assertTrue(settings.pet.enabled)
        self.assertTrue(settings.pet.deterministic)
        self.assertFalse(settings.pet.live2d_native_enabled)
        self.assertEqual(settings.pet.live2d_sdk_root, "")
        self.assertEqual(settings.pet.asset_root, "")
        self.assertFalse(settings.pet.cloud_vision_enabled)
        self.assertFalse(settings.pet.voice_clone_enabled)

    def test_global_pet_env_aliases_merge_into_pet_config(self):
        with patched_env(
            MEMOCHAT_ENABLE_PET="false",
            MEMOCHAT_PET_DETERMINISTIC="false",
            MEMOCHAT_ENABLE_LIVE2D_NATIVE="true",
            MEMOCHAT_LIVE2D_SDK_ROOT="/data/third_party/live2d/CubismSdkForNative-current",
            MEMOCHAT_PET_ASSET_ROOT="/data/memochat/pet-assets",
            MEMOCHAT_PET_CLOUD_VISION="true",
            MEMOCHAT_PET_VOICE_CLONE="true",
        ):
            settings = Settings.from_yaml("config.yaml")

        self.assertFalse(settings.pet.enabled)
        self.assertFalse(settings.pet.deterministic)
        self.assertTrue(settings.pet.live2d_native_enabled)
        self.assertEqual(settings.pet.live2d_sdk_root, "/data/third_party/live2d/CubismSdkForNative-current")
        self.assertEqual(settings.pet.asset_root, "/data/memochat/pet-assets")
        self.assertTrue(settings.pet.cloud_vision_enabled)
        self.assertTrue(settings.pet.voice_clone_enabled)

    async def test_runtime_rejects_new_sessions_when_pet_is_disabled(self):
        runtime = PetRuntime(PetRuntimeConfig(enabled=False))

        with self.assertRaisesRegex(RuntimeError, "disabled"):
            await runtime.create_session(uid=7)

    async def test_runtime_fails_fast_when_deterministic_runtime_is_disabled(self):
        runtime = PetRuntime(PetRuntimeConfig(enabled=True, deterministic=False))

        with self.assertRaisesRegex(RuntimeError, "no provider adapter"):
            await runtime.create_session(uid=7, provider="scripted")


if __name__ == "__main__":
    unittest.main()
