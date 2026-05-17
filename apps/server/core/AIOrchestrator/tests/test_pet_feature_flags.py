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
        self.assertFalse(settings.pet.local_vision_enabled)
        self.assertEqual(settings.pet.vision_camera_index, 0)
        self.assertEqual(settings.pet.vision_analyzer, "opencv")
        self.assertEqual(settings.pet.face_landmarker_model_path, "")
        self.assertEqual(settings.pet.object_detector_model_path, "")
        self.assertFalse(settings.pet.vision_retain_raw_frames)
        self.assertFalse(settings.pet.voice_clone_enabled)
        self.assertEqual(settings.pet.voice_provider, "scripted")
        self.assertEqual(settings.pet.voice_sovits_base_url, "")
        self.assertEqual(settings.pet.voice_sovits_reference_audio, "")
        self.assertEqual(settings.pet.voice_sovits_output_dir, "/app/.data/pet-voice-cache")
        self.assertTrue(settings.pet.voice_training_enabled)
        self.assertEqual(settings.pet.voice_training_artifact_root, "/app/.data/pet-voice-training")

    def test_global_pet_env_aliases_merge_into_pet_config(self):
        with patched_env(
            MEMOCHAT_ENABLE_PET="false",
            MEMOCHAT_PET_DETERMINISTIC="false",
            MEMOCHAT_ENABLE_LIVE2D_NATIVE="true",
            MEMOCHAT_LIVE2D_SDK_ROOT="/data/third_party/live2d/CubismSdkForNative-current",
            MEMOCHAT_PET_ASSET_ROOT="/data/memochat/pet-assets",
            MEMOCHAT_PET_CLOUD_VISION="true",
            MEMOCHAT_PET_LOCAL_VISION="true",
            MEMOCHAT_PET_VISION_CAMERA_INDEX="2",
            MEMOCHAT_PET_VISION_ANALYZER="opencv",
            MEMOCHAT_PET_FACE_LANDMARKER_MODEL="/data/third_party/mediapipe/face_landmarker.task",
            MEMOCHAT_PET_OBJECT_DETECTOR_MODEL="/data/third_party/mediapipe/object_detector.tflite",
            MEMOCHAT_PET_VISION_RETAIN_RAW_FRAMES="true",
            MEMOCHAT_PET_VOICE_CLONE="true",
            MEMOCHAT_PET_VOICE_PROVIDER="gpt-sovits",
            MEMOCHAT_PET_SOVITS_BASE_URL="http://127.0.0.1:9880",
            MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO="/data/memochat/voice/ref.wav",
            MEMOCHAT_PET_SOVITS_OUTPUT_DIR="/data/docker-data/memochat/pet-voice-cache",
            MEMOCHAT_PET_VOICE_TRAINING="false",
            MEMOCHAT_PET_VOICE_TRAINING_ARTIFACT_ROOT="/data/docker-data/memochat/pet-voice-training",
        ):
            settings = Settings.from_yaml("config.yaml")

        self.assertFalse(settings.pet.enabled)
        self.assertFalse(settings.pet.deterministic)
        self.assertTrue(settings.pet.live2d_native_enabled)
        self.assertEqual(settings.pet.live2d_sdk_root, "/data/third_party/live2d/CubismSdkForNative-current")
        self.assertEqual(settings.pet.asset_root, "/data/memochat/pet-assets")
        self.assertTrue(settings.pet.cloud_vision_enabled)
        self.assertTrue(settings.pet.local_vision_enabled)
        self.assertEqual(settings.pet.vision_camera_index, 2)
        self.assertEqual(settings.pet.vision_analyzer, "opencv")
        self.assertEqual(settings.pet.face_landmarker_model_path, "/data/third_party/mediapipe/face_landmarker.task")
        self.assertEqual(settings.pet.object_detector_model_path, "/data/third_party/mediapipe/object_detector.tflite")
        self.assertTrue(settings.pet.vision_retain_raw_frames)
        self.assertTrue(settings.pet.voice_clone_enabled)
        self.assertEqual(settings.pet.voice_provider, "gpt-sovits")
        self.assertEqual(settings.pet.voice_sovits_base_url, "http://127.0.0.1:9880")
        self.assertEqual(settings.pet.voice_sovits_reference_audio, "/data/memochat/voice/ref.wav")
        self.assertEqual(settings.pet.voice_sovits_output_dir, "/data/docker-data/memochat/pet-voice-cache")
        self.assertFalse(settings.pet.voice_training_enabled)
        self.assertEqual(settings.pet.voice_training_artifact_root, "/data/docker-data/memochat/pet-voice-training")

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
