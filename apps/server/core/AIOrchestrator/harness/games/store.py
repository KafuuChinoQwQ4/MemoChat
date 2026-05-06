from __future__ import annotations

import json
from typing import Any, Protocol

try:
    import structlog
except ModuleNotFoundError:
    class _FallbackLogger:
        def info(self, *args, **kwargs):
            return None

        def warning(self, *args, **kwargs):
            return None

    class _StructlogFallback:
        def get_logger(self):
            return _FallbackLogger()

    structlog = _StructlogFallback()

from harness.games.contracts import GameState, GameTemplate

logger = structlog.get_logger()


class GameStateStore(Protocol):
    async def startup(self) -> None:
        ...

    async def load_rooms(self) -> list[GameState]:
        ...

    async def save_room(self, state: GameState) -> None:
        ...

    async def delete_room(self, room_id: str, deleted_at: int) -> None:
        ...

    async def list_templates(self, uid: int) -> list[GameTemplate]:
        ...

    async def save_template(self, template: GameTemplate) -> None:
        ...

    async def get_template(self, template_id: str, uid: int = 0) -> GameTemplate | None:
        ...

    async def delete_template(self, template_id: str, uid: int, deleted_at: int) -> None:
        ...


class NoopGameStateStore:
    async def startup(self) -> None:
        return None

    async def load_rooms(self) -> list[GameState]:
        return []

    async def save_room(self, state: GameState) -> None:
        return None

    async def delete_room(self, room_id: str, deleted_at: int) -> None:
        return None

    async def list_templates(self, uid: int) -> list[GameTemplate]:
        return []

    async def save_template(self, template: GameTemplate) -> None:
        return None

    async def get_template(self, template_id: str, uid: int = 0) -> GameTemplate | None:
        return None

    async def delete_template(self, template_id: str, uid: int, deleted_at: int) -> None:
        return None


class PostgresGameStateStore:
    async def startup(self) -> None:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        await pg.execute(
            """
            CREATE TABLE IF NOT EXISTS ai_game_room_state (
                room_id VARCHAR(64) PRIMARY KEY,
                owner_uid INTEGER NOT NULL,
                ruleset_id VARCHAR(64) NOT NULL,
                status VARCHAR(32) NOT NULL,
                phase VARCHAR(32) NOT NULL,
                title TEXT NOT NULL DEFAULT '',
                snapshot_json JSONB NOT NULL,
                created_at BIGINT NOT NULL,
                updated_at BIGINT NOT NULL,
                deleted_at BIGINT DEFAULT NULL
            )
            """
        )
        await pg.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_ai_game_room_state_owner_updated
            ON ai_game_room_state (owner_uid, updated_at DESC)
            """
        )
        await pg.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_ai_game_room_state_ruleset_updated
            ON ai_game_room_state (ruleset_id, updated_at DESC)
            """
        )
        await pg.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_ai_game_room_state_status_updated
            ON ai_game_room_state (status, updated_at DESC)
            """
        )
        await pg.execute(
            """
            CREATE TABLE IF NOT EXISTS ai_game_template (
                template_id VARCHAR(64) PRIMARY KEY,
                uid INTEGER NOT NULL,
                title TEXT NOT NULL DEFAULT '',
                description TEXT NOT NULL DEFAULT '',
                ruleset_id VARCHAR(64) NOT NULL DEFAULT 'werewolf.basic',
                max_players INTEGER NOT NULL DEFAULT 12,
                agents_json JSONB NOT NULL DEFAULT '[]'::jsonb,
                config_json JSONB NOT NULL DEFAULT '{}'::jsonb,
                metadata_json JSONB NOT NULL DEFAULT '{}'::jsonb,
                created_at BIGINT NOT NULL,
                updated_at BIGINT NOT NULL,
                deleted_at BIGINT DEFAULT NULL
            )
            """
        )
        await pg.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_ai_game_template_uid_updated
            ON ai_game_template (uid, updated_at DESC)
            """
        )
        await pg.execute(
            """
            CREATE INDEX IF NOT EXISTS idx_ai_game_template_ruleset_updated
            ON ai_game_template (ruleset_id, updated_at DESC)
            """
        )

    async def load_rooms(self) -> list[GameState]:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        rows = await pg.fetchall(
            """
            SELECT snapshot_json
            FROM ai_game_room_state
            WHERE deleted_at IS NULL
            ORDER BY updated_at DESC
            """
        )
        rooms: list[GameState] = []
        for row in rows:
            snapshot = _decode_json(row.get("snapshot_json"))
            try:
                rooms.append(GameState.from_snapshot(snapshot))
            except Exception as exc:
                room_id = snapshot.get("room", {}).get("room_id", "") if isinstance(snapshot, dict) else ""
                logger.warning("game.persistence.snapshot_decode_failed", room_id=room_id, error=str(exc))
        return rooms

    async def save_room(self, state: GameState) -> None:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        snapshot = state.to_snapshot()
        room = state.room
        await pg.execute(
            """
            INSERT INTO ai_game_room_state
            (room_id, owner_uid, ruleset_id, status, phase, title, snapshot_json, created_at, updated_at, deleted_at)
            VALUES ($1, $2, $3, $4, $5, $6, $7::jsonb, $8, $9, NULL)
            ON CONFLICT (room_id) DO UPDATE SET
                owner_uid = EXCLUDED.owner_uid,
                ruleset_id = EXCLUDED.ruleset_id,
                status = EXCLUDED.status,
                phase = EXCLUDED.phase,
                title = EXCLUDED.title,
                snapshot_json = EXCLUDED.snapshot_json,
                updated_at = EXCLUDED.updated_at,
                deleted_at = NULL
            WHERE ai_game_room_state.updated_at <= EXCLUDED.updated_at
            """,
            room.room_id,
            room.owner_uid,
            room.ruleset_id,
            room.status,
            room.phase,
            room.title,
            json.dumps(snapshot, ensure_ascii=False),
            room.created_at,
            room.updated_at,
        )

    async def delete_room(self, room_id: str, deleted_at: int) -> None:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        await pg.execute(
            """
            UPDATE ai_game_room_state
            SET deleted_at = $2, updated_at = $2
            WHERE room_id = $1
            """,
            room_id,
            deleted_at,
        )

    async def list_templates(self, uid: int) -> list[GameTemplate]:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        rows = await pg.fetchall(
            """
            SELECT template_id, uid, title, description, ruleset_id, max_players,
                   agents_json, config_json, metadata_json, created_at, updated_at
            FROM ai_game_template
            WHERE deleted_at IS NULL AND ($1::integer <= 0 OR uid = $1)
            ORDER BY updated_at DESC
            """,
            uid,
        )
        return [_template_from_row(row) for row in rows]

    async def save_template(self, template: GameTemplate) -> None:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        await pg.execute(
            """
            INSERT INTO ai_game_template
            (template_id, uid, title, description, ruleset_id, max_players,
             agents_json, config_json, metadata_json, created_at, updated_at, deleted_at)
            VALUES ($1, $2, $3, $4, $5, $6, $7::jsonb, $8::jsonb, $9::jsonb, $10, $11, NULL)
            ON CONFLICT (template_id) DO UPDATE SET
                uid = EXCLUDED.uid,
                title = EXCLUDED.title,
                description = EXCLUDED.description,
                ruleset_id = EXCLUDED.ruleset_id,
                max_players = EXCLUDED.max_players,
                agents_json = EXCLUDED.agents_json,
                config_json = EXCLUDED.config_json,
                metadata_json = EXCLUDED.metadata_json,
                updated_at = EXCLUDED.updated_at,
                deleted_at = NULL
            WHERE ai_game_template.updated_at <= EXCLUDED.updated_at
            """,
            template.template_id,
            template.uid,
            template.title,
            template.description,
            template.ruleset_id,
            template.max_players,
            json.dumps(template.agents, ensure_ascii=False),
            json.dumps(template.config, ensure_ascii=False),
            json.dumps(template.metadata, ensure_ascii=False),
            template.created_at,
            template.updated_at,
        )

    async def get_template(self, template_id: str, uid: int = 0) -> GameTemplate | None:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        row = await pg.fetchone(
            """
            SELECT template_id, uid, title, description, ruleset_id, max_players,
                   agents_json, config_json, metadata_json, created_at, updated_at
            FROM ai_game_template
            WHERE template_id = $1 AND deleted_at IS NULL AND ($2::integer <= 0 OR uid = $2)
            """,
            template_id,
            uid,
        )
        return _template_from_row(row) if row else None

    async def delete_template(self, template_id: str, uid: int, deleted_at: int) -> None:
        from db.postgres_client import PostgresClient

        pg = PostgresClient()
        await pg.execute(
            """
            UPDATE ai_game_template
            SET deleted_at = $3, updated_at = $3
            WHERE template_id = $1 AND uid = $2
            """,
            template_id,
            uid,
            deleted_at,
        )


def _decode_json(raw_value: Any) -> dict[str, Any]:
    if raw_value is None:
        return {}
    if isinstance(raw_value, dict):
        return raw_value
    if isinstance(raw_value, str):
        try:
            decoded = json.loads(raw_value)
            return decoded if isinstance(decoded, dict) else {}
        except Exception:
            return {}
    return {}


def _decode_list(raw_value: Any) -> list[dict[str, Any]]:
    if raw_value is None:
        return []
    if isinstance(raw_value, list):
        return [item for item in raw_value if isinstance(item, dict)]
    if isinstance(raw_value, str):
        try:
            decoded = json.loads(raw_value)
            return [item for item in decoded if isinstance(item, dict)] if isinstance(decoded, list) else []
        except Exception:
            return []
    return []


def _template_from_row(row: dict[str, Any]) -> GameTemplate:
    return GameTemplate(
        template_id=str(row.get("template_id") or ""),
        uid=int(row.get("uid") or 0),
        title=str(row.get("title") or ""),
        description=str(row.get("description") or ""),
        ruleset_id=str(row.get("ruleset_id") or "werewolf.basic"),
        max_players=int(row.get("max_players") or 12),
        agents=_decode_list(row.get("agents_json")),
        config=_decode_json(row.get("config_json")),
        metadata=_decode_json(row.get("metadata_json")),
        created_at=int(row.get("created_at") or 0),
        updated_at=int(row.get("updated_at") or 0),
    )
