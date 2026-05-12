from __future__ import annotations

import time
import uuid

from .contracts import PetSession


class PetSessionStore:
    """In-memory session lifecycle store for the deterministic pet runtime."""

    def __init__(self) -> None:
        self._sessions: dict[str, PetSession] = {}
        self._last_timestamp_ms = 0

    def create(
        self,
        uid: int = 0,
        profile_id: str = "default",
        persona: str = "memo-pet",
        provider: str = "scripted",
    ) -> PetSession:
        now = self._now_ms()
        session = PetSession(
            session_id=f"pet-{uuid.uuid4().hex}",
            uid=uid,
            profile_id=profile_id or "default",
            persona=persona or "memo-pet",
            provider=provider or "scripted",
            created_at_ms=now,
            updated_at_ms=now,
        )
        self._sessions[session.session_id] = session
        return session

    def create_session(
        self,
        uid: int = 0,
        profile_id: str = "default",
        persona: str = "memo-pet",
        provider: str = "scripted",
    ) -> PetSession:
        return self.create(uid=uid, profile_id=profile_id, persona=persona, provider=provider)

    def get(self, session_id: str) -> PetSession | None:
        return self._sessions.get(session_id)

    def require(self, session_id: str) -> PetSession:
        session = self.get(session_id)
        if session is None:
            raise KeyError(f"pet session not found: {session_id}")
        return session

    def require_session(self, session_id: str) -> PetSession:
        return self.require(session_id)

    def touch(self, session_id: str, status: str | None = None) -> PetSession:
        session = self.require(session_id)
        session.updated_at_ms = self._now_ms()
        if status:
            session.status = status
        return session

    def touch_session(self, session_id: str, status: str | None = None) -> PetSession:
        return self.touch(session_id, status=status)

    def list(self, uid: int = 0) -> list[PetSession]:
        sessions = list(self._sessions.values())
        if uid > 0:
            sessions = [session for session in sessions if session.uid == uid]
        return sorted(sessions, key=lambda item: item.updated_at_ms, reverse=True)

    def list_sessions(self, uid: int = 0) -> list[PetSession]:
        return self.list(uid=uid)

    def _now_ms(self) -> int:
        now = int(time.time() * 1000)
        if now <= self._last_timestamp_ms:
            now = self._last_timestamp_ms + 1
        self._last_timestamp_ms = now
        return now
