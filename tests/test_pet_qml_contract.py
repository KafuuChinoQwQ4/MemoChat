import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
PET_MODEL_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/PetModel.h"
PET_MODEL_CPP = REPO_ROOT / "apps/client/desktop/MemoChat-qml/PetModel.cpp"
PET_CONTROLLER_H = REPO_ROOT / "apps/client/desktop/MemoChat-qml/PetController.h"
PET_SCENE_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetScene.qml"
PET_WINDOW_QML = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml/pet/PetWindow.qml"
QML_QRC = REPO_ROOT / "apps/client/desktop/MemoChat-qml/qml.qrc"


class PetQmlContractTests(unittest.TestCase):
    def test_pet_model_exposes_v1_event_metadata(self):
        header = PET_MODEL_H.read_text(encoding="utf-8")
        controller = PET_CONTROLLER_H.read_text(encoding="utf-8")

        for prop in ("schemaVersion", "eventId", "turnId", "phase"):
            self.assertIn(prop, header)
            self.assertIn(prop, controller)

    def test_pet_model_parses_v1_nested_fields_with_legacy_fallbacks(self):
        source = PET_MODEL_CPP.read_text(encoding="utf-8")

        for token in (
            'QStringLiteral("animation")',
            'QStringLiteral("audio")',
            'QStringLiteral("text")',
            'QStringLiteral("delta")',
            'QStringLiteral("display")',
            'QStringLiteral("final")',
            'QStringLiteral("speech")',
            'QStringLiteral("lip_sync")',
        ):
            self.assertIn(token, source)

        self.assertIn("_speech_turn_id", source)
        self.assertIn("jsonBool", source)

    def test_pet_scene_keeps_null_controller_and_phase_status_guards(self):
        scene = PET_SCENE_QML.read_text(encoding="utf-8")

        self.assertIn('expression: root.petController ? root.petController.expression : "neutral"', scene)
        self.assertIn('motion: root.petController ? root.petController.motion : "idle"', scene)
        self.assertIn("function phaseText", scene)
        self.assertIn("function phaseColor", scene)
        self.assertIn("function displayStatus", scene)
        self.assertIn("root.petController && root.petController.error.length > 0", scene)

    def test_pet_window_and_resources_remain_registered(self):
        window = PET_WINDOW_QML.read_text(encoding="utf-8")
        qrc = QML_QRC.read_text(encoding="utf-8")

        self.assertIn("Qt.WindowStaysOnTopHint", window)
        self.assertIn("root.startSystemMove()", window)
        self.assertIn("qml/pet/PetWindow.qml", qrc)
        self.assertIn("qml/pet/PetScene.qml", qrc)
        self.assertIn("qml/pet/Live2DCharacterPane.qml", qrc)
        self.assertIn('alias="icons/modelive2d.png"', qrc)


if __name__ == "__main__":
    unittest.main()
