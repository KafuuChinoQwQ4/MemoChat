from __future__ import annotations

import base64
import binascii
import math
import os
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
        face_landmarker_model_path: str = "",
        object_detector_model_path: str = "",
        retain_raw_frames: bool = False,
    ) -> None:
        self._enabled = bool(enabled)
        self._default_camera_index = max(0, int(default_camera_index or 0))
        self._analyzer = str(analyzer or "opencv")
        self._face_landmarker_model_path = str(face_landmarker_model_path or "")
        self._object_detector_model_path = str(object_detector_model_path or "")
        self._retain_raw_frames = bool(retain_raw_frames)

    def diagnostics(self, camera_index: int | None = None, open_camera: bool = False) -> dict[str, Any]:
        requested_index = self._default_camera_index if camera_index is None or camera_index < 0 else int(camera_index)
        requested_analyzer = _normalize_analyzer(self._analyzer)
        resolved_model_path = _resolve_face_landmarker_model_path(self._face_landmarker_model_path)
        resolved_object_model_path = _resolve_object_detector_model_path(self._object_detector_model_path)
        mediapipe_available = _mediapipe_available()
        mediapipe_ready = (
            requested_analyzer == "mediapipe_face_landmarker"
            and bool(resolved_model_path)
            and mediapipe_available
        )
        object_detector_ready = bool(resolved_object_model_path) and mediapipe_available
        if object_detector_ready:
            object_detector_status = "ready"
        elif self._object_detector_model_path and not resolved_object_model_path:
            object_detector_status = "model_missing"
        elif resolved_object_model_path and not mediapipe_available:
            object_detector_status = "mediapipe_unavailable"
        else:
            object_detector_status = "heuristic_fallback"
        result: dict[str, Any] = {
            "enabled": self._enabled,
            "default_camera_index": self._default_camera_index,
            "camera_index": requested_index,
            "analyzer": self._analyzer,
            "requested_analyzer": requested_analyzer,
            "face_landmarker_model_path": self._face_landmarker_model_path,
            "face_landmarker_model_resolved": resolved_model_path,
            "object_detector_model_path": self._object_detector_model_path,
            "object_detector_model_resolved": resolved_object_model_path,
            "mediapipe_available": mediapipe_available,
            "face_landmarker_ready": mediapipe_ready,
            "object_detector_available": mediapipe_available,
            "object_detector_ready": object_detector_ready,
            "object_detector_status": object_detector_status,
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
            result["ready"] = bool(result["camera_devices"]) and (requested_analyzer != "mediapipe_face_landmarker" or mediapipe_ready)
            result["status"] = "camera_probe_skipped"
            if requested_analyzer == "mediapipe_face_landmarker" and not mediapipe_ready:
                if not mediapipe_available:
                    result["status"] = "mediapipe_unavailable"
                    result["message"] = (
                        "MediaPipe is not installed; install mediapipe and set face_landmarker_model_path or "
                        "place face_landmarker.task under /data/third_party/mediapipe."
                    )
                else:
                    result["status"] = "mediapipe_model_missing"
                    result["message"] = (
                        "MediaPipe Face Landmarker model is not configured; set face_landmarker_model_path or "
                        "place face_landmarker.task under /data/third_party/mediapipe."
                    )
                return result
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
            if requested_analyzer == "mediapipe_face_landmarker" and not mediapipe_ready:
                result["ready"] = False
                if not mediapipe_available:
                    result["status"] = "mediapipe_unavailable"
                    result["message"] = "OpenCV can read frames, but MediaPipe is not installed."
                else:
                    result["status"] = "mediapipe_model_missing"
                    result["message"] = (
                        "OpenCV can read frames, but MediaPipe Face Landmarker is not configured."
                    )
                return result
            result["ready"] = True
            result["status"] = "ready"
            result["message"] = "Local camera capture is ready."
            return result
        finally:
            if capture is not None:
                capture.release()

    def capture(self, request: VisionCaptureRequest) -> PetObservation:
        if not self._enabled and not request.has_frame_payload:
            raise VisionAnalyzerError("local vision capture is disabled by configuration")
        try:
            import cv2
        except ImportError as exc:  # pragma: no cover - exercised only on lean images.
            raise VisionAnalyzerError("opencv-python is required for local vision capture") from exc

        requested_analyzer = request.analyzer or ""
        configured_analyzer = self._analyzer or "opencv"
        requested_mode = _normalize_analyzer(requested_analyzer)
        configured_mode = _normalize_analyzer(configured_analyzer)
        effective_analyzer = configured_analyzer if requested_mode == "opencv" and configured_mode != "opencv" else (
            requested_analyzer or configured_analyzer
        )
        analyzer_mode = _normalize_analyzer(effective_analyzer)
        model_path = str(
            (request.metadata.get("face_landmarker_model_path") if isinstance(request.metadata, dict) else "")
            or self._face_landmarker_model_path
        )
        object_detector_model_path = str(
            (request.metadata.get("object_detector_model_path") if isinstance(request.metadata, dict) else "")
            or self._object_detector_model_path
        )
        if request.has_frame_payload:
            frame = _decode_frame_payload(request, cv2)
            height, width = frame.shape[:2]
            vision = _analyze_frame(
                frame,
                effective_analyzer,
                face_landmarker_model_path=model_path,
                object_detector_model_path=object_detector_model_path,
            )
            vision.update(
                {
                    "enabled": True,
                    "analyzer": analyzer_mode,
                    "requested_analyzer": requested_analyzer or configured_analyzer,
                    "source": "uploaded_frame",
                    "frame": {"width": int(width), "height": int(height)},
                    "captured_at_ms": int(time.time() * 1000),
                }
            )
            if request.frame_mime:
                vision["frame_mime"] = request.frame_mime
            _apply_request_metadata_to_vision(vision, request.metadata)
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
            vision = _analyze_frame(
                frame,
                effective_analyzer,
                face_landmarker_model_path=model_path,
                object_detector_model_path=object_detector_model_path,
            )
            vision.update(
                {
                    "enabled": True,
                    "analyzer": analyzer_mode,
                    "requested_analyzer": requested_analyzer or configured_analyzer,
                    "source": "camera_frame",
                    "camera_index": camera_index,
                    "frame": {"width": int(width), "height": int(height)},
                    "captured_at_ms": int(time.time() * 1000),
                }
            )
            _apply_request_metadata_to_vision(vision, request.metadata)
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


def _analyze_frame(
    frame,
    analyzer: str,
    face_landmarker_model_path: str = "",
    object_detector_model_path: str = "",
) -> dict[str, Any]:
    normalized = _normalize_analyzer(analyzer)
    base = _opencv_brightness_analysis(frame)
    if normalized != "mediapipe_face_landmarker":
        vision = _opencv_face_analysis(frame, base)
    else:
        model_path = _resolve_face_landmarker_model_path(face_landmarker_model_path)
        if not model_path:
            base["mode"] = "opencv_face_detection"
            base["attention"] = "mediapipe_model_missing"
            vision = _opencv_face_analysis(frame, base)
        else:
            try:
                vision = _mediapipe_face_landmarker_analysis(frame, base, model_path)
            except ImportError:
                base["mode"] = "opencv_face_detection"
                base["attention"] = "mediapipe_unavailable"
                vision = _opencv_face_analysis(frame, base)
            except Exception as exc:
                base["mode"] = "opencv_face_detection"
                base["attention"] = f"mediapipe_error:{type(exc).__name__}"
                vision = _opencv_face_analysis(frame, base)

    return _augment_scene_and_objects(frame, vision, object_detector_model_path=object_detector_model_path)


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
        "confidence": 0.0,
        "pose": {"brightness": round(max(0.0, min(1.0, brightness)), 3)},
        "scene": {"lighting": attention, "brightness": round(max(0.0, min(1.0, brightness)), 3)},
        "objects": [],
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
    face_box = {
        "x": round(float(x) / max(1, width), 4),
        "y": round(float(y) / max(1, height), 4),
        "width": round(float(w) / max(1, width), 4),
        "height": round(float(h) / max(1, height), 4),
    }
    return {
        **fallback,
        "mode": "opencv_face_detection",
        "face_present": True,
        "attention": "face_detected",
        "expression": "neutral",
        "confidence": 0.72,
        "pose": {
            **dict(fallback.get("pose") or {}),
            "face_box": face_box,
            "face_confidence": 0.72,
        },
        "scene": {
            **dict(fallback.get("scene") or {}),
            "face_coverage": round(max(0.0, min(1.0, (w * h) / max(1.0, width * height))), 4),
        },
    }


def _mediapipe_face_landmarker_analysis(frame, fallback: dict[str, Any], model_path: str) -> dict[str, Any]:
    import cv2
    import mediapipe as mp

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    rgb = _ensure_contiguous_frame(rgb)
    landmarker_options = mp.tasks.vision.FaceLandmarkerOptions(
        base_options=mp.tasks.BaseOptions(model_asset_path=model_path),
        running_mode=mp.tasks.vision.RunningMode.IMAGE,
        num_faces=1,
        output_face_blendshapes=True,
        output_facial_transformation_matrixes=True,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5,
    )
    landmarker = mp.tasks.vision.FaceLandmarker.create_from_options(landmarker_options)
    try:
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
        result = landmarker.detect(mp_image)
    finally:
        close = getattr(landmarker, "close", None)
        if callable(close):
            close()

    return _mediapipe_face_landmarker_result(frame, result, fallback)


def _mediapipe_face_landmarker_result(frame, result, fallback: dict[str, Any]) -> dict[str, Any]:
    landmarks = list(getattr(result, "face_landmarks", []) or [])
    if not landmarks:
        return {
            **fallback,
            "mode": "mediapipe_face_landmarker",
            "face_present": False,
            "attention": str(fallback.get("attention") or "no_face"),
            "expression": "",
            "confidence": 0.0,
            "gesture": "",
            "pose": {
                **dict(fallback.get("pose") or {}),
                "face_landmark_count": 0,
            },
            "head_pose": {},
            "blendshapes": {},
            "scene": dict(fallback.get("scene") or {}),
            "objects": list(fallback.get("objects") or []),
        }

    face_landmarks = list(landmarks[0] or [])
    blendshapes = _face_blendshape_scores(getattr(result, "face_blendshapes", []) or [])
    head_pose = _head_pose_from_matrix(getattr(result, "facial_transformation_matrixes", []) or [])
    face_box = _landmark_face_box(face_landmarks, frame)
    face_center = _landmark_center(face_box)
    expression = _expression_from_blendshapes(blendshapes)
    gesture = _gesture_from_blendshapes(blendshapes)
    scene = dict(fallback.get("scene") or {})
    scene["lighting"] = str(scene.get("lighting") or fallback.get("attention") or "ambient")

    pose = dict(fallback.get("pose") or {})
    pose["face_box"] = face_box
    pose["face_center"] = face_center
    pose["face_landmark_count"] = len(face_landmarks)
    pose["face_confidence"] = 1.0

    return {
        **fallback,
        "mode": "mediapipe_face_landmarker",
        "face_present": True,
        "attention": "user_face",
        "expression": expression,
        "confidence": 1.0,
        "pose": pose,
        "head_pose": head_pose,
        "blendshapes": blendshapes,
        "scene": scene,
        "objects": list(fallback.get("objects") or []),
        "gesture": gesture,
    }


def _augment_scene_and_objects(
    frame,
    vision: dict[str, Any],
    object_detector_model_path: str = "",
) -> dict[str, Any]:
    payload = dict(vision or {})
    scene = dict(payload.get("scene") or {})
    objects = _list_or_empty(payload.get("objects"))
    detector_status = "heuristic_fallback"
    resolved_object_model_path = _resolve_object_detector_model_path(object_detector_model_path)
    if resolved_object_model_path:
        if _mediapipe_available():
            try:
                detected_objects = _mediapipe_object_detector_analysis(frame, resolved_object_model_path)
                if detected_objects:
                    objects = detected_objects
                detector_status = "ready"
            except ImportError:
                detector_status = "mediapipe_unavailable"
            except Exception as exc:
                detector_status = f"mediapipe_error:{type(exc).__name__}"
        else:
            detector_status = "mediapipe_unavailable"
    elif object_detector_model_path:
        detector_status = "model_missing"

    labels = _object_labels_from_objects(objects)
    scene["summary"] = _scene_summary_text(payload, scene, labels)
    scene["object_count"] = len(objects)
    scene["object_labels"] = labels
    scene["top_object_label"] = labels[0] if labels else ""
    scene["object_detector_ready"] = detector_status == "ready"
    scene["object_detector_status"] = detector_status
    payload["scene"] = scene
    payload["objects"] = objects
    return payload


def _scene_summary_text(vision: dict[str, Any], scene: dict[str, Any], object_labels: list[str]) -> str:
    existing = _normalize_scene_text(scene.get("summary") or scene.get("description") or scene.get("caption") or "")
    if existing:
        return existing

    face_present = bool(vision.get("face_present", False))
    lighting = str(scene.get("lighting") or vision.get("attention") or "").strip().lower()
    if object_labels:
        label_text = _join_scene_labels(object_labels)
        if face_present:
            return f"看到了你的脸，旁边还有{label_text}。"
        return f"画面里有{label_text}。"
    if face_present:
        if lighting in {"dim", "dark"}:
            return "看到了你的脸，周围光线有点暗。"
        if lighting in {"bright", "sunny"}:
            return "看到了你的脸，周围很明亮。"
        return "看到了你的脸，画面比较安静。"
    if lighting in {"dim", "dark"}:
        return "画面里没有明显物体，光线有点暗。"
    if lighting in {"bright", "sunny"}:
        return "画面里没有明显物体，周围很明亮。"
    return "画面里没有明显物体。"


def _object_labels_from_objects(objects) -> list[str]:
    labels: list[str] = []
    seen: set[str] = set()
    for item in list(objects or [])[:5]:
        if not isinstance(item, dict):
            continue
        label = str(item.get("label") or item.get("name") or item.get("category_name") or "").strip()
        if not label or label in seen:
            continue
        seen.add(label)
        labels.append(label)
    return labels


def _join_scene_labels(labels: list[str]) -> str:
    cleaned = [str(label).strip() for label in labels if str(label).strip()]
    if not cleaned:
        return ""
    if len(cleaned) == 1:
        return cleaned[0]
    return "、".join(cleaned)


def _normalize_scene_text(value: Any) -> str:
    return " ".join(str(value or "").strip().split())


def _list_or_empty(value) -> list[Any]:
    if isinstance(value, list):
        return list(value)
    if isinstance(value, tuple):
        return list(value)
    return []


def _mediapipe_object_detector_analysis(frame, model_path: str) -> list[dict[str, Any]]:
    import cv2
    import mediapipe as mp

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    rgb = _ensure_contiguous_frame(rgb)
    detector_options = mp.tasks.vision.ObjectDetectorOptions(
        base_options=mp.tasks.BaseOptions(model_asset_path=model_path),
        running_mode=mp.tasks.vision.RunningMode.IMAGE,
        max_results=5,
        score_threshold=0.25,
    )
    detector = mp.tasks.vision.ObjectDetector.create_from_options(detector_options)
    try:
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
        result = detector.detect(mp_image)
    finally:
        close = getattr(detector, "close", None)
        if callable(close):
            close()
    return _mediapipe_object_detector_result(frame, result)


def _mediapipe_object_detector_result(frame, result) -> list[dict[str, Any]]:
    detections = list(getattr(result, "detections", []) or [])
    if not detections:
        return []

    objects: list[dict[str, Any]] = []
    for index, detection in enumerate(detections, start=1):
        categories = list(getattr(detection, "categories", []) or [])
        label, category_name, score = _detection_label_and_score(categories, fallback_label=f"object_{index}")
        bbox = _detection_bounding_box(getattr(detection, "bounding_box", None), frame)
        item: dict[str, Any] = {
            "label": label,
            "name": label,
            "score": score,
            "source": "mediapipe_object_detector",
        }
        if category_name and category_name != label:
            item["category_name"] = category_name
        if bbox:
            item["bounding_box"] = bbox
        objects.append(item)

    objects.sort(key=lambda item: float(item.get("score", 0.0) or 0.0), reverse=True)
    return objects


def _detection_label_and_score(categories, fallback_label: str = "object") -> tuple[str, str, float]:
    for category in categories or []:
        if isinstance(category, dict):
            name = str(category.get("category_name") or category.get("display_name") or category.get("name") or "").strip()
            score = _clamp_float(category.get("score"), 0.0, 1.0, 0.0)
        else:
            name = str(
                getattr(category, "category_name", "")
                or getattr(category, "display_name", "")
                or getattr(category, "name", "")
                or ""
            ).strip()
            score = _clamp_float(getattr(category, "score", 0.0), 0.0, 1.0, 0.0)
        if name:
            return name, name, score
    return fallback_label, "", 0.0


def _detection_bounding_box(bounding_box, frame) -> dict[str, Any]:
    if bounding_box is None:
        return {}

    frame_height, frame_width = frame.shape[:2]
    raw_x = _box_value(bounding_box, "origin_x", "x", "left", "xmin", default=0.0)
    raw_y = _box_value(bounding_box, "origin_y", "y", "top", "ymin", default=0.0)
    raw_width = _box_value(bounding_box, "width", "w", default=0.0)
    raw_height = _box_value(bounding_box, "height", "h", default=0.0)
    normalized = max(abs(raw_x), abs(raw_y), abs(raw_width), abs(raw_height)) <= 1.0
    if normalized:
        x = _clamp_float(raw_x, 0.0, 1.0, 0.0)
        y = _clamp_float(raw_y, 0.0, 1.0, 0.0)
        width = _clamp_float(raw_width, 0.0, 1.0, 0.0)
        height = _clamp_float(raw_height, 0.0, 1.0, 0.0)
    else:
        x = _clamp_float(raw_x, 0.0, float(frame_width), 0.0) / max(1.0, float(frame_width))
        y = _clamp_float(raw_y, 0.0, float(frame_height), 0.0) / max(1.0, float(frame_height))
        width = _clamp_float(raw_width, 0.0, float(frame_width), 0.0) / max(1.0, float(frame_width))
        height = _clamp_float(raw_height, 0.0, float(frame_height), 0.0) / max(1.0, float(frame_height))
    return {
        "x": round(float(x), 4),
        "y": round(float(y), 4),
        "width": round(float(width), 4),
        "height": round(float(height), 4),
        "pixel_x": int(round(float(x) * max(1, frame_width))),
        "pixel_y": int(round(float(y) * max(1, frame_height))),
        "pixel_width": int(round(float(width) * max(1, frame_width))),
        "pixel_height": int(round(float(height) * max(1, frame_height))),
    }


def _box_value(source, *names: str, default: float = 0.0) -> float:
    if isinstance(source, dict):
        for name in names:
            if name in source and source[name] is not None:
                return _clamp_float(source[name], -1_000_000.0, 1_000_000.0, default)
        return default
    for name in names:
        if hasattr(source, name):
            value = getattr(source, name)
            if value is not None:
                return _clamp_float(value, -1_000_000.0, 1_000_000.0, default)
    return default


def _face_blendshape_scores(result_blendshapes) -> dict[str, float]:
    scores: dict[str, float] = {}
    for face_blendshapes in list(result_blendshapes or [])[:1]:
        categories = getattr(face_blendshapes, "categories", face_blendshapes) or []
        for category in categories:
            name = str(
                getattr(category, "category_name", "")
                or getattr(category, "display_name", "")
                or getattr(category, "name", "")
                or ""
            ).strip()
            if not name:
                continue
            scores[name] = round(_clamp_float(getattr(category, "score", 0.0), 0.0, 1.0, 0.0), 4)
    return scores


def _clamp_float(value, minimum: float, maximum: float, default: float = 0.0) -> float:
    try:
        numeric = float(value)
    except (TypeError, ValueError):
        return float(default)
    if math.isnan(numeric) or math.isinf(numeric):
        return float(default)
    return max(minimum, min(maximum, numeric))


def _expression_from_blendshapes(blendshapes: dict[str, float]) -> str:
    smile = max(
        blendshapes.get("mouthSmileLeft", 0.0),
        blendshapes.get("mouthSmileRight", 0.0),
        blendshapes.get("smile", 0.0),
    )
    jaw_open = blendshapes.get("jawOpen", 0.0)
    blink = max(
        blendshapes.get("eyeBlinkLeft", 0.0),
        blendshapes.get("eyeBlinkRight", 0.0),
    )
    brow_down = max(
        blendshapes.get("browDownLeft", 0.0),
        blendshapes.get("browDownRight", 0.0),
        blendshapes.get("browInnerDown", 0.0),
    )
    brow_up = max(
        blendshapes.get("browInnerUp", 0.0),
        blendshapes.get("browOuterUpLeft", 0.0),
        blendshapes.get("browOuterUpRight", 0.0),
    )
    if smile >= 0.55:
        return "smile"
    if jaw_open >= 0.45 and blink <= 0.35:
        return "surprised"
    if brow_down >= 0.45:
        return "concerned"
    if blink >= 0.7 and smile < 0.2:
        return "tired"
    if brow_up >= 0.45:
        return "curious"
    return "neutral"


def _gesture_from_blendshapes(blendshapes: dict[str, float]) -> str:
    jaw_open = blendshapes.get("jawOpen", 0.0)
    blink = max(
        blendshapes.get("eyeBlinkLeft", 0.0),
        blendshapes.get("eyeBlinkRight", 0.0),
    )
    if jaw_open >= 0.45:
        return "talking"
    if blink >= 0.75:
        return "blink"
    return ""


def _head_pose_from_matrix(matrices) -> dict[str, Any]:
    matrix_rows = _matrix_rows(matrices)
    if not matrix_rows:
        return {}

    rotation = [row[:3] for row in matrix_rows[:3]]
    if len(rotation) < 3 or any(len(row) < 3 for row in rotation):
        return {"matrix": matrix_rows}

    r00, r01, r02 = rotation[0][:3]
    r10, r11, r12 = rotation[1][:3]
    r20, r21, r22 = rotation[2][:3]
    sy = math.sqrt(r00 * r00 + r10 * r10)
    singular = sy < 1e-6
    if not singular:
        pitch = math.atan2(r21, r22)
        yaw = math.atan2(-r20, sy)
        roll = math.atan2(r10, r00)
    else:
        pitch = math.atan2(-r12, r11)
        yaw = math.atan2(-r20, sy)
        roll = 0.0

    translation = {
        "x": round(float(matrix_rows[0][3]), 4),
        "y": round(float(matrix_rows[1][3]), 4),
        "z": round(float(matrix_rows[2][3]), 4),
    }
    return {
        "matrix": matrix_rows,
        "yaw": round(math.degrees(yaw), 3),
        "pitch": round(math.degrees(pitch), 3),
        "roll": round(math.degrees(roll), 3),
        "translation": translation,
    }


def _landmark_face_box(landmarks, frame) -> dict[str, float]:
    height, width = frame.shape[:2]
    xs = [float(getattr(point, "x", 0.0)) for point in landmarks]
    ys = [float(getattr(point, "y", 0.0)) for point in landmarks]
    if not xs or not ys:
        return {}
    xmin = max(0.0, min(xs))
    ymin = max(0.0, min(ys))
    xmax = min(1.0, max(xs))
    ymax = min(1.0, max(ys))
    box = {
        "x": round(xmin, 4),
        "y": round(ymin, 4),
        "width": round(max(0.0, xmax - xmin), 4),
        "height": round(max(0.0, ymax - ymin), 4),
        "pixel_width": int(round(max(0.0, xmax - xmin) * max(1, width))),
        "pixel_height": int(round(max(0.0, ymax - ymin) * max(1, height))),
    }
    return box


def _landmark_center(face_box: dict[str, float]) -> dict[str, float]:
    if not face_box:
        return {}
    return {
        "x": round(float(face_box.get("x", 0.0)) + float(face_box.get("width", 0.0)) / 2.0, 4),
        "y": round(float(face_box.get("y", 0.0)) + float(face_box.get("height", 0.0)) / 2.0, 4),
    }


def _matrix_rows(matrices) -> list[list[float]]:
    if not matrices:
        return []
    first = matrices[0]
    raw = first.tolist() if hasattr(first, "tolist") else list(first)
    if len(raw) == 16:
        rows = [raw[i : i + 4] for i in range(0, 16, 4)]
    elif len(raw) == 4 and all(hasattr(row, "__iter__") for row in raw):
        rows = [list(row)[:4] for row in raw]
    else:
        return []
    normalized_rows: list[list[float]] = []
    for row in rows:
        normalized_rows.append([round(float(value), 6) for value in row[:4]])
    return normalized_rows


def _normalize_analyzer(value: str) -> str:
    normalized = str(value or "opencv").strip().lower().replace("-", "_")
    if "mediapipe" in normalized and "landmarker" in normalized:
        return "mediapipe_face_landmarker"
    if normalized in {"mediapipe", "face_landmarker", "face_landmarker_mediapipe"}:
        return "mediapipe_face_landmarker"
    if "opencv" in normalized:
        return "opencv"
    return normalized or "opencv"


def _resolve_face_landmarker_model_path(configured_path: str) -> str:
    candidates = []
    normalized_configured = str(configured_path or "").strip()
    if normalized_configured:
        candidates.append(normalized_configured)
    env_configured = os.environ.get("MEMOCHAT_PET_FACE_LANDMARKER_MODEL", "").strip()
    if env_configured and env_configured not in candidates:
        candidates.append(env_configured)
    candidates.extend(
        [
            "/data/third_party/mediapipe/face_landmarker.task",
            "/data/third_party/mediapipe/face_landmarker_v2_with_blendshapes.task",
        ]
    )
    for raw_candidate in candidates:
        candidate = Path(raw_candidate).expanduser()
        if candidate.is_file():
            try:
                return str(candidate.resolve())
            except OSError:
                return str(candidate)
        if candidate.is_dir():
            for nested_name in (
                "face_landmarker.task",
                "face_landmarker_v2_with_blendshapes.task",
            ):
                nested = candidate / nested_name
                if nested.is_file():
                    try:
                        return str(nested.resolve())
                    except OSError:
                        return str(nested)
    return ""


def _resolve_object_detector_model_path(configured_path: str) -> str:
    candidates = []
    normalized_configured = str(configured_path or "").strip()
    if normalized_configured:
        candidates.append(normalized_configured)
    env_configured = os.environ.get("MEMOCHAT_PET_OBJECT_DETECTOR_MODEL", "").strip()
    if env_configured and env_configured not in candidates:
        candidates.append(env_configured)
    candidates.extend(
        [
            "/data/third_party/mediapipe/lite-model_efficientdet_lite0_detection_metadata_1.tflite",
            "/data/third_party/mediapipe/object_detector.tflite",
            "/data/third_party/mediapipe/efficientdet_lite0.tflite",
        ]
    )
    for raw_candidate in candidates:
        candidate = Path(raw_candidate).expanduser()
        if candidate.is_file():
            try:
                return str(candidate.resolve())
            except OSError:
                return str(candidate)
        if candidate.is_dir():
            for nested_name in (
                "lite-model_efficientdet_lite0_detection_metadata_1.tflite",
                "object_detector.tflite",
                "efficientdet_lite0.tflite",
            ):
                nested = candidate / nested_name
                if nested.is_file():
                    try:
                        return str(nested.resolve())
                    except OSError:
                        return str(nested)
    return ""


def _mediapipe_available() -> bool:
    try:
        import mediapipe  # noqa: F401
    except ImportError:
        return False
    return True


def _ensure_contiguous_frame(frame):
    try:
        import numpy as np
    except ImportError:
        return frame
    return np.ascontiguousarray(frame)


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


def _apply_request_metadata_to_vision(vision: dict[str, Any], metadata: dict[str, Any]) -> None:
    if not isinstance(metadata, dict):
        return
    for key in ("reply_language", "language", "voice_language", "text_lang", "speech_rules"):
        value = str(metadata.get(key) or "").strip()
        if value:
            vision[key] = value


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
