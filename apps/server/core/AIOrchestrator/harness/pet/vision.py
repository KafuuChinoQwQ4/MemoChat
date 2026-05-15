from __future__ import annotations

import base64
import binascii
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .contracts import PetObservation


class VisionAnalyzerError(RuntimeError):
    def __init__(self, message: str, recoverable: bool = True) -> None:
        super().__init__(message)
        self.recoverable = recoverable


@dataclass(frozen=True)
class VisionCaptureRequest:
    session_id: str
    camera_index: int = -1
    analyzer: str = "opencv"
    include_frame: bool = False
    frame_base64: str = ""
    frame_mime: str = ""
    frame_width: int = 0
    frame_height: int = 0
    metadata: dict[str, Any] = field(default_factory=dict)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> "VisionCaptureRequest":
        camera_index = data.get("camera_index")
        frame_base64, frame_mime = _split_frame_data_url(data.get("frame_base64"), data.get("frame_mime"))
        return cls(
            session_id=str(data.get("session_id") or ""),
            camera_index=_camera_index(camera_index),
            analyzer=str(data.get("analyzer") or "opencv"),
            include_frame=data.get("include_frame") is True,
            frame_base64=frame_base64,
            frame_mime=frame_mime,
            frame_width=_non_negative_int(data.get("frame_width")),
            frame_height=_non_negative_int(data.get("frame_height")),
            metadata=dict(data.get("metadata")) if isinstance(data.get("metadata"), dict) else {},
        )

    @property
    def has_frame_payload(self) -> bool:
        return bool(self.frame_base64.strip())


class LocalVisionAnalyzer:
    """Captures one local camera frame and emits structured pet observation data."""

    def __init__(
        self,
        enabled: bool = False,
        default_camera_index: int = 0,
        analyzer: str = "opencv",
        retain_raw_frames: bool = False,
    ) -> None:
        self._enabled = bool(enabled)
        self._default_camera_index = max(0, int(default_camera_index or 0))
        self._analyzer = str(analyzer or "opencv")
        self._retain_raw_frames = bool(retain_raw_frames)

    def diagnostics(self, camera_index: int | None = None, open_camera: bool = False) -> dict[str, Any]:
        requested_index = self._default_camera_index if camera_index is None or camera_index < 0 else int(camera_index)
        result: dict[str, Any] = {
            "enabled": self._enabled,
            "default_camera_index": self._default_camera_index,
            "camera_index": requested_index,
            "analyzer": self._analyzer,
            "opencv_available": False,
            "camera_devices": _video_devices(),
            "camera_open_tested": bool(open_camera),
            "camera_available": False,
            "frame_captured": False,
            "frame": {},
            "ready": False,
            "status": "disabled" if not self._enabled else "not_tested",
            "message": "",
        }
        if not self._enabled:
            result["message"] = "Local vision capture is disabled by configuration."
            return result

        try:
            import cv2
        except ImportError:
            result["status"] = "opencv_missing"
            result["message"] = "opencv-python is required for local vision capture."
            return result

        result["opencv_available"] = True
        result["opencv_version"] = str(getattr(cv2, "__version__", ""))
        if not open_camera:
            result["ready"] = bool(result["camera_devices"])
            result["status"] = "camera_probe_skipped"
            result["message"] = (
                "Camera open test was skipped; pass open_camera=true to verify OpenCV can read one frame."
            )
            return result

        capture = cv2.VideoCapture(requested_index)
        try:
            if not capture or not capture.isOpened():
                result["status"] = "camera_unavailable"
                result["message"] = f"Camera index {requested_index} is not available to OpenCV."
                return result
            result["camera_available"] = True
            ok, frame = capture.read()
            if not ok or frame is None:
                result["status"] = "frame_capture_failed"
                result["message"] = "OpenCV opened the camera but could not read a frame."
                return result
            height, width = frame.shape[:2]
            result["frame_captured"] = True
            result["frame"] = {"width": int(width), "height": int(height)}
            result["ready"] = True
            result["status"] = "ready"
            result["message"] = "Local camera capture is ready."
            return result
        finally:
            if capture is not None:
                capture.release()

    def capture(self, request: VisionCaptureRequest) -> PetObservation:
        if not self._enabled:
            raise VisionAnalyzerError("local vision capture is disabled by configuration")
        try:
            import cv2
        except ImportError as exc:  # pragma: no cover - exercised only on lean images.
            raise VisionAnalyzerError("opencv-python is required for local vision capture") from exc

        if request.has_frame_payload:
            frame = _decode_frame_payload(request, cv2)
            height, width = frame.shape[:2]
            vision = _analyze_frame(frame, request.analyzer or self._analyzer)
            vision.update(
                {
                    "enabled": True,
                    "source": "uploaded_frame",
                    "frame": {"width": int(width), "height": int(height)},
                    "captured_at_ms": int(time.time() * 1000),
                }
            )
            if request.frame_mime:
                vision["frame_mime"] = request.frame_mime
            if request.frame_width > 0 or request.frame_height > 0:
                vision["client_frame"] = {
                    "width": request.frame_width,
                    "height": request.frame_height,
                }
            privacy = {
                "camera_used": True,
                "raw_frame_sent": False,
                "raw_frame_received": True,
                "raw_audio_recorded": False,
                "cloud_vision_used": False,
                "retention": "debug" if (request.include_frame and self._retain_raw_frames) else "none",
            }
            if request.include_frame and self._retain_raw_frames:
                privacy["raw_frame_retained"] = True
            return PetObservation(
                session_id=request.session_id,
                audio={"vad": "idle", "rms": 0.0, "interrupt": False},
                vision=vision,
                privacy=privacy,
            )

        camera_index = request.camera_index if request.camera_index >= 0 else self._default_camera_index
        capture = cv2.VideoCapture(camera_index)
        try:
            if not capture or not capture.isOpened():
                raise VisionAnalyzerError(f"camera is not available: index {camera_index}")
            ok, frame = capture.read()
            if not ok or frame is None:
                raise VisionAnalyzerError("camera frame capture failed")
            height, width = frame.shape[:2]
            vision = _analyze_frame(frame, request.analyzer or self._analyzer)
            vision.update(
                {
                    "enabled": True,
                    "camera_index": camera_index,
                    "frame": {"width": int(width), "height": int(height)},
                    "captured_at_ms": int(time.time() * 1000),
                }
            )
            privacy = {
                "camera_used": True,
                "raw_frame_sent": False,
                "raw_audio_recorded": False,
                "cloud_vision_used": False,
                "retention": "debug" if (request.include_frame and self._retain_raw_frames) else "none",
            }
            if request.include_frame and self._retain_raw_frames:
                privacy["raw_frame_sent"] = False
                privacy["raw_frame_retained"] = True
            return PetObservation(
                session_id=request.session_id,
                audio={"vad": "idle", "rms": 0.0, "interrupt": False},
                vision=vision,
                privacy=privacy,
            )
        finally:
            if capture is not None:
                capture.release()


def _analyze_frame(frame, analyzer: str) -> dict[str, Any]:
    normalized = str(analyzer or "opencv").strip().lower()
    base = _opencv_brightness_analysis(frame)
    if "mediapipe" not in normalized:
        return _opencv_face_analysis(frame, base)
    try:
        return _mediapipe_face_analysis(frame, base)
    except ImportError:
        base["mode"] = "opencv_frame_stats"
        base["attention"] = "mediapipe_unavailable"
        return base
    except Exception as exc:
        base["mode"] = "opencv_frame_stats"
        base["attention"] = f"mediapipe_error:{type(exc).__name__}"
        return base


def _opencv_brightness_analysis(frame) -> dict[str, Any]:
    try:
        import cv2

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        brightness = float(gray.mean()) / 255.0
    except Exception:
        brightness = 0.0
    attention = "bright" if brightness >= 0.58 else "dim" if brightness <= 0.22 else "ambient"
    return {
        "enabled": True,
        "mode": "opencv_frame_stats",
        "face_present": False,
        "attention": attention,
        "expression": "",
        "pose": {"brightness": round(max(0.0, min(1.0, brightness)), 3)},
        "gesture": "",
    }


def _opencv_face_analysis(frame, fallback: dict[str, Any]) -> dict[str, Any]:
    try:
        import cv2

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        cascade_path = cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
        cascade = cv2.CascadeClassifier(cascade_path)
        faces = cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=4, minSize=(48, 48))
    except Exception as exc:
        return {**fallback, "attention": f"opencv_face_error:{type(exc).__name__}"}

    if len(faces) == 0:
        return {**fallback, "mode": "opencv_face_detection", "face_present": False}

    height, width = frame.shape[:2]
    x, y, w, h = max(faces, key=lambda face: int(face[2]) * int(face[3]))
    return {
        **fallback,
        "mode": "opencv_face_detection",
        "face_present": True,
        "attention": "face_detected",
        "expression": "neutral",
        "pose": {
            **dict(fallback.get("pose") or {}),
            "face_box": {
                "x": round(float(x) / max(1, width), 4),
                "y": round(float(y) / max(1, height), 4),
                "width": round(float(w) / max(1, width), 4),
                "height": round(float(h) / max(1, height), 4),
            },
        },
    }


def _mediapipe_face_analysis(frame, fallback: dict[str, Any]) -> dict[str, Any]:
    import cv2
    import mediapipe as mp

    image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    face_detection = mp.solutions.face_detection.FaceDetection(model_selection=0, min_detection_confidence=0.5)
    try:
        result = face_detection.process(image)
    finally:
        face_detection.close()

    detections = list(result.detections or [])
    if not detections:
        return {**fallback, "mode": "mediapipe_face_detection", "face_present": False}

    detection = detections[0]
    score = float(detection.score[0]) if detection.score else 0.0
    box = detection.location_data.relative_bounding_box
    return {
        **fallback,
        "mode": "mediapipe_face_detection",
        "face_present": True,
        "attention": "face_detected",
        "expression": "neutral",
        "pose": {
            **dict(fallback.get("pose") or {}),
            "face_confidence": round(max(0.0, min(1.0, score)), 3),
            "face_box": {
                "x": round(float(box.xmin), 4),
                "y": round(float(box.ymin), 4),
                "width": round(float(box.width), 4),
                "height": round(float(box.height), 4),
            },
        },
    }


def _camera_index(value: Any) -> int:
    if value is None:
        return -1
    try:
        number = int(value)
    except (TypeError, ValueError):
        return -1
    return number if number >= 0 else -1


def _decode_frame_payload(request: VisionCaptureRequest, cv2):
    try:
        import numpy as np
    except ImportError as exc:  # pragma: no cover - OpenCV wheels include numpy in normal environments.
        raise VisionAnalyzerError("numpy is required to decode uploaded vision frames") from exc

    raw_text = request.frame_base64.strip()
    if not raw_text:
        raise VisionAnalyzerError("uploaded vision frame is empty")
    try:
        raw = base64.b64decode(raw_text, validate=True)
    except (binascii.Error, ValueError) as exc:
        raise VisionAnalyzerError("uploaded vision frame is not valid base64") from exc
    if not raw:
        raise VisionAnalyzerError("uploaded vision frame is empty")
    if len(raw) > 8 * 1024 * 1024:
        raise VisionAnalyzerError("uploaded vision frame is too large")

    encoded = np.frombuffer(raw, dtype=np.uint8)
    frame = cv2.imdecode(encoded, getattr(cv2, "IMREAD_COLOR", 1))
    if frame is None or getattr(frame, "size", 0) == 0:
        raise VisionAnalyzerError("uploaded vision frame could not be decoded")
    return frame


def _split_frame_data_url(frame_base64: Any, frame_mime: Any) -> tuple[str, str]:
    text = str(frame_base64 or "").strip()
    mime = str(frame_mime or "").strip().lower()
    if text.lower().startswith("data:") and "," in text:
        header, text = text.split(",", 1)
        header = header[5:]
        if ";" in header:
            maybe_mime = header.split(";", 1)[0].strip().lower()
        else:
            maybe_mime = header.strip().lower()
        if maybe_mime:
            mime = mime or maybe_mime
    return text.strip(), mime


def _non_negative_int(value: Any) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError):
        return 0
    return max(0, number)


def _video_devices() -> list[str]:
    return sorted(str(path) for path in Path("/dev").glob("video*"))
