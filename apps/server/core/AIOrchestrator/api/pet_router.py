from __future__ import annotations

import asyncio
import json
from typing import Any, AsyncIterator

import structlog
from config import settings
from fastapi import APIRouter, HTTPException
from fastapi.responses import FileResponse, StreamingResponse
from harness.pet import (
    PetObservation,
    PetRuntime,
    VisionAnalyzerError,
    VisionCaptureRequest,
    VisionSegmentRequest,
    VoiceTrainingRequest,
)
from pydantic import BaseModel, Field

logger = structlog.get_logger()
router = APIRouter()
_runtime = PetRuntime(settings.pet)


class PetSessionCreateReq(BaseModel):
    uid: int = 0
    profile_id: str = "default"
    persona: str = "memo-pet"
    provider: str = "scripted"


class PetInputReq(BaseModel):
    uid: int = 0
    content: str
    model_type: str = ""
    model_name: str = ""
    metadata: dict[str, Any] = Field(default_factory=dict)


class PetObservationReq(BaseModel):
    type: str = "pet.observation"
    session_id: str = ""
    audio: dict[str, Any] = Field(default_factory=dict)
    vision: dict[str, Any] = Field(default_factory=dict)
    privacy: dict[str, Any] = Field(default_factory=dict)


class PetVisionCaptureReq(BaseModel):
    camera_index: int | None = None
    analyzer: str = "opencv"
    include_frame: bool = False
    frame_base64: str = ""
    frame_mime: str = ""
    frame_width: int = 0
    frame_height: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class PetVisionSegmentFrameReq(BaseModel):
    frame_base64: str = ""
    frame_mime: str = ""
    frame_width: int = 0
    frame_height: int = 0
    t_ms: int = 0
    metadata: dict[str, Any] = Field(default_factory=dict)


class PetVisionSegmentCaptureReq(BaseModel):
    analyzer: str = "opencv"
    include_frame: bool = False
    segment_id: str = ""
    duration_ms: int = 0
    frames: list[PetVisionSegmentFrameReq] = Field(default_factory=list)
    metadata: dict[str, Any] = Field(default_factory=dict)


class PetVoiceTrainingReq(BaseModel):
    uid: int = 0
    profile_id: str = "default"
    voice_name: str = "Kafuuchino-voice"
    language: str = "zh-CN"
    reference_audio_path: str
    reference_audio_directory: str = ""
    reference_audio_file: str = ""
    consent_confirmed: bool = False
    consent_scope: str = "local_default_reference"
    source: str = "src-default"
    provider: str = "gpt-sovits"
    metadata: dict[str, Any] = Field(default_factory=dict)


@router.get("/diagnostics")
async def diagnostics(probe_voice_endpoint: bool = False, open_camera: bool = False):
    return {
        "code": 0,
        "message": "ok",
        "diagnostics": await _runtime.diagnostics(
            probe_voice_endpoint=probe_voice_endpoint,
            open_camera=open_camera,
        ),
    }


@router.get("/diagnostics/voice")
async def voice_diagnostics(probe_endpoint: bool = False):
    return {
        "code": 0,
        "message": "ok",
        "diagnostics": await _runtime.voice_diagnostics(probe_endpoint=probe_endpoint),
    }


@router.get("/diagnostics/vision")
async def vision_diagnostics(camera_index: int | None = None, open_camera: bool = False):
    return {
        "code": 0,
        "message": "ok",
        "diagnostics": _runtime.vision_diagnostics(camera_index=camera_index, open_camera=open_camera),
    }


@router.post("/sessions")
async def create_session(req: PetSessionCreateReq):
    if not settings.pet.enabled:
        raise HTTPException(status_code=404, detail="desktop pet is disabled by configuration")
    try:
        session = await _runtime.create_session(
            uid=req.uid,
            profile_id=req.profile_id,
            persona=req.persona,
            provider=req.provider,
        )
    except RuntimeError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return {"code": 0, "message": "ok", "session": session.to_dict()}


@router.get("/sessions")
async def list_sessions(uid: int = 0):
    return {
        "code": 0,
        "message": "ok",
        "sessions": [session.to_dict() for session in _runtime.list_sessions(uid)],
    }


@router.post("/voice-training/jobs")
async def create_voice_training_job(req: PetVoiceTrainingReq):
    try:
        job = _runtime.create_voice_training_job(VoiceTrainingRequest.from_dict(req.model_dump()))
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc
    except RuntimeError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return {"code": 0, "message": "ok", "job": job.to_dict()}


@router.get("/voice-training/jobs")
async def list_voice_training_jobs(uid: int = 0):
    return {
        "code": 0,
        "message": "ok",
        "jobs": [job.to_dict() for job in _runtime.list_voice_training_jobs(uid)],
    }


@router.get("/voice-training/jobs/{job_id}")
async def get_voice_training_job(job_id: str):
    try:
        job = _runtime.get_voice_training_job(job_id)
    except KeyError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return {"code": 0, "message": "ok", "job": job.to_dict()}


@router.get("/audio/{file_name}")
async def get_voice_audio(file_name: str):
    try:
        path = _runtime.audio_file_path(file_name)
    except KeyError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return FileResponse(path)


@router.post("/sessions/{session_id}/input")
async def submit_input(session_id: str, req: PetInputReq):
    if not req.content.strip():
        raise HTTPException(status_code=400, detail="content cannot be empty")
    try:
        events = await _runtime.submit_input(
            session_id=session_id,
            content=req.content,
            model_type=req.model_type,
            model_name=req.model_name,
            metadata=req.metadata,
        )
    except KeyError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return {"code": 0, "message": "ok", "events": [event.to_dict() for event in events]}


@router.post("/sessions/{session_id}/observation")
async def submit_observation(session_id: str, req: PetObservationReq):
    payload = req.model_dump()
    payload["session_id"] = session_id
    try:
        event = await _runtime.update_observation(PetObservation.from_dict(payload))
    except KeyError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return {"code": 0, "message": "ok", "event": event.to_dict()}


@router.post("/sessions/{session_id}/capture")
async def capture_observation(session_id: str, req: PetVisionCaptureReq):
    payload = req.model_dump()
    payload["session_id"] = session_id
    try:
        observation, event = await _runtime.capture_observation(VisionCaptureRequest.from_dict(payload))
    except KeyError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    except VisionAnalyzerError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return {
        "code": 0,
        "message": "ok",
        "observation": observation.to_dict(),
        "event": event.to_dict(),
    }


@router.post("/sessions/{session_id}/capture-segment")
async def capture_segment_observation(session_id: str, req: PetVisionSegmentCaptureReq):
    payload = req.model_dump()
    payload["session_id"] = session_id
    try:
        observation, event = await _runtime.capture_segment_observation(VisionSegmentRequest.from_dict(payload))
    except KeyError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    except VisionAnalyzerError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return {
        "code": 0,
        "message": "ok",
        "observation": observation.to_dict(),
        "event": event.to_dict(),
    }


@router.post("/sessions/{session_id}/interrupt")
async def interrupt(session_id: str):
    try:
        event = await _runtime.interrupt(session_id)
    except KeyError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return {"code": 0, "message": "ok", "event": event.to_dict()}


@router.get("/sessions/{session_id}/stream")
async def stream_events(session_id: str):
    if _runtime.get_session(session_id) is None:
        raise HTTPException(status_code=404, detail="pet session not found")

    async def event_generator() -> AsyncIterator[bytes]:
        try:
            async for event in _runtime.stream(session_id):
                yield f"data: {json.dumps(event.to_dict(), ensure_ascii=False)}\n\n".encode("utf-8")
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            logger.error("pet.stream.error", session_id=session_id, error=str(exc))
            payload = {"type": "pet.error", "session_id": session_id, "message": str(exc)}
            yield f"data: {json.dumps(payload, ensure_ascii=False)}\n\n".encode("utf-8")

    return StreamingResponse(
        event_generator(),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive",
            "X-Accel-Buffering": "no",
        },
    )
