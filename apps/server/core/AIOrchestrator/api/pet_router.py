from __future__ import annotations

import asyncio
import json
from typing import Any, AsyncIterator

import structlog
from fastapi import APIRouter, HTTPException
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field

from harness.pet import PetObservation, PetRuntime

logger = structlog.get_logger()
router = APIRouter()
_runtime = PetRuntime()


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


@router.post("/sessions")
async def create_session(req: PetSessionCreateReq):
    session = await _runtime.create_session(
        uid=req.uid,
        profile_id=req.profile_id,
        persona=req.persona,
        provider=req.provider,
    )
    return {"code": 0, "message": "ok", "session": session.to_dict()}


@router.get("/sessions")
async def list_sessions(uid: int = 0):
    return {
        "code": 0,
        "message": "ok",
        "sessions": [session.to_dict() for session in _runtime.list_sessions(uid)],
    }


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
