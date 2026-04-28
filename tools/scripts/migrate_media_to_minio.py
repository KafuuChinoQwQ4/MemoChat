# migrate_media_to_minio.py
# Migrates local media files to MinIO object storage.
# Supports dry-run mode for safe preview before live migration.
#
# Usage:
#   Dry-run:  python migrate_media_to_minio.py --dry-run
#   Live:     python migrate_media_to_minio.py
#
# Prerequisites:
#   - Docker Desktop running with memochat-minio container
#   - PostgreSQL accessible (host/port/user/pass from Memo_ops/config/opsserver.yaml)
#   - Python 3 with: pip install psycopg2-binary python-dotenv minio

import argparse
import json
import os
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import psycopg2
except ImportError:
    print("ERROR: psycopg2 not installed. Run: pip install psycopg2-binary")
    sys.exit(1)

try:
    from minio import Minio
except ImportError:
    print("ERROR: minio not installed. Run: pip install minio")
    sys.exit(1)


# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).parent.parent.resolve()
OPS_CONFIG = REPO_ROOT / "Memo_ops/config/opsserver.yaml"
UPLOADS_ROOT = REPO_ROOT / "Memo_ops/runtime/services/GateServer/uploads"
LOG_DIR = REPO_ROOT / "Memo_ops/artifacts/logs/media-migration"

# MinIO settings (matches config.ini [MinIO] section)
MINIO_ENDPOINT = "127.0.0.1:9000"
MINIO_ACCESS_KEY = os.environ.get("MINIO_ACCESS_KEY", "memochat_admin")
MINIO_SECRET_KEY = os.environ.get("MINIO_SECRET_KEY", "MinioPass2026!")
MINIO_PUBLIC_URL = "http://127.0.0.1:9000"

BUCKETS = {
    "avatar": "memochat-avatar",
    "file": "memochat-file",
    "image": "memochat-image",
    "video": "memochat-video",
}

# Extensions -> bucket
EXT_BUCKET_MAP = {
    ".jpg": "image", ".jpeg": "image", ".png": "image", ".gif": "image",
    ".webp": "image", ".bmp": "image", ".svg": "image",
    ".mp4": "video", ".avi": "video", ".mov": "video", ".mkv": "video",
    ".flv": "video", ".wmv": "video", ".webm": "video",
    ".mp3": "video", ".wav": "video", ".ogg": "video",
    ".flac": "video", ".aac": "video", ".m4a": "video",
}


def load_pg_config():
    import yaml
    with open(OPS_CONFIG, encoding="utf-8") as f:
        config = yaml.safe_load(f)
    pg = config.get("postgresql", {})
    return {
        "host": pg.get("host", "127.0.0.1"),
        "port": pg.get("port", 15432),
        "user": pg.get("user", "memochat"),
        "password": pg.get("password", ""),
        "database": pg.get("database", "memo_pg"),
        # App schema for business tables (chat_media_asset, user, etc.)
        "schema": "memo",
    }


def get_bucket(media_type: str, media_key: str) -> str:
    t = (media_type or "").lower().strip()
    if t in ("avatar",):       return BUCKETS["avatar"]
    if t in ("image",):       return BUCKETS["image"]
    if t in ("video", "audio"): return BUCKETS["video"]
    # Fallback: infer from file extension in media_key
    ext = Path(media_key).suffix.lower()
    if ext in EXT_BUCKET_MAP:
        return BUCKETS[EXT_BUCKET_MAP[ext]]
    return BUCKETS["file"]


def resolve_local_path(storage_path: str, uploads_root: Path) -> Path | None:
    if not storage_path:
        return None
    # Already a full URL
    if storage_path.startswith("http://") or storage_path.startswith("https://"):
        return None
    # Absolute path
    if Path(storage_path).is_absolute():
        p = Path(storage_path)
    # Relative starting with "uploads/"
    elif storage_path.startswith("uploads/"):
        p = uploads_root / storage_path
    else:
        p = uploads_root / "uploads" / storage_path
    if p.exists():
        return p
    return None


def make_s3_key(origin_file_name: str, media_key: str, ts_ms: int) -> str:
    dt = datetime.fromtimestamp(ts_ms / 1000)
    date_tag = dt.strftime("%Y%m%d")
    safe_name = "".join(c if c.isalnum() or c in "_-." else "_" for c in origin_file_name)
    if len(safe_name) > 96:
        safe_name = safe_name[-96:]
    return f"assets/{date_tag}/{media_key}_{safe_name}"


def sanitize_key(key: str) -> str:
    return key.replace("'", "''")


# ---------------------------------------------------------------------------
# MinIO client
# ---------------------------------------------------------------------------
def get_minio_client():
    return Minio(
        MINIO_ENDPOINT,
        access_key=MINIO_ACCESS_KEY,
        secret_key=MINIO_SECRET_KEY,
        secure=False,
    )


def ensure_alias_in_mc():
    """Verify mc alias is configured. docker exec the minio container."""
    result = subprocess.run(
        ["docker", "exec", "memochat-minio", "mc", "alias", "list"],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        raise RuntimeError(f"mc alias list failed: {result.stderr}")
    return True


# ---------------------------------------------------------------------------
# Main migration
# ---------------------------------------------------------------------------
def run_migration(dry_run: bool):
    mode = "DRY-RUN" if dry_run else "LIVE"
    print(f"\n{'='*60}")
    print(f"  MemoChat Media Migration to MinIO  [{mode}]")
    print(f"{'='*60}\n")
    print(f"PostgreSQL : {OPS_CONFIG}")
    print(f"MinIO      : {MINIO_ENDPOINT}")
    print(f"Uploads    : {UPLOADS_ROOT}")
    print(f"")

    # Load PG config
    pg = load_pg_config()
    print(f"[Config] PG {pg['host']}:{pg['port']}/{pg['database']} schema={pg['schema']}")

    # Connect to PostgreSQL
    conn = psycopg2.connect(
        host=pg["host"], port=pg["port"],
        user=pg["user"], password=pg["password"],
        database=pg["database"],
        options=f"-c search_path={pg['schema']}"
    )
    conn.autocommit = True
    cur = conn.cursor()

    # Verify MinIO reachable
    try:
        mc = get_minio_client()
        mc.list_buckets()
        print(f"[Config] MinIO connected: {MINIO_ENDPOINT}")
    except Exception as e:
        print(f"[WARN] MinIO not reachable: {e} (continuing for dry-run)")

    # ---------------------------------------------------------------------------
    # Step 1: Count records
    # ---------------------------------------------------------------------------
    print(f"\n--- Step 1: Scanning chat_media_asset ---")
    cur.execute("""
        SELECT storage_provider, status, COUNT(*) AS cnt
        FROM chat_media_asset
        GROUP BY storage_provider, status
        ORDER BY storage_provider, status
    """)
    for row in cur.fetchall():
        print(f"  {row[0]} / status={row[1]} -> {row[2]} rows")

    cur.execute("""
        SELECT COUNT(*) FROM chat_media_asset
        WHERE storage_provider = 'local' AND status = 1
    """)
    local_count = cur.fetchone()[0]
    print(f"\nRecords to migrate (local, active): {local_count}")

    if local_count == 0:
        print("No local media records found. Nothing to migrate.")
        return

    # ---------------------------------------------------------------------------
    # Step 2: Backup
    # ---------------------------------------------------------------------------
    if not dry_run:
        print(f"\n--- Step 2: Creating backup table ---")
        backup_ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_table = f"chat_media_asset_backup_{backup_ts}"
        cur.execute(f"""
            CREATE TABLE IF NOT EXISTS {backup_table} AS
            SELECT * FROM chat_media_asset WHERE storage_provider = 'local'
        """)
        conn.commit()
        cur.execute(f"SELECT COUNT(*) FROM {backup_table}")
        backup_rows = cur.fetchone()[0]
        print(f"Backup table created: {backup_table} ({backup_rows} rows)")

    # ---------------------------------------------------------------------------
    # Step 3: Create log dir
    # ---------------------------------------------------------------------------
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = LOG_DIR / f"migration_{ts}.log"

    def log(line: str):
        stamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        entry = f"[{stamp}] {line}"
        print(entry)
        with open(log_file, "a", encoding="utf-8") as f:
            f.write(entry + "\n")

    log(f"MODE: {mode}")
    log(f"Log file: {log_file}")
    log(f"Records to migrate: {local_count}")
    log("-" * 60)

    # ---------------------------------------------------------------------------
    # Step 4: Migrate media records
    # ---------------------------------------------------------------------------
    print(f"\n--- Step 3: Migrating media records ---")
    log("Starting media record migration...")

    cur.execute("""
        SELECT media_id, media_key, media_type, origin_file_name,
               mime, size_bytes, storage_path, created_at_ms
        FROM chat_media_asset
        WHERE storage_provider = 'local' AND status = 1
        ORDER BY created_at_ms
    """)

    ok = skip = fail = 0
    total_bytes = 0
    minio_client = None
    if not dry_run:
        try:
            minio_client = get_minio_client()
        except Exception as e:
            log(f"ERROR: MinIO client init failed: {e}")
            raise

    for row in cur.fetchall():
        media_id, media_key, media_type, origin_file_name, mime, size_bytes, storage_path, created_at_ms = row

        local_path = resolve_local_path(storage_path, UPLOADS_ROOT)
        if local_path is None:
            log(f"SKIP|{media_key}|no local file found at storage_path='{storage_path}'")
            skip += 1
            continue

        bucket = get_bucket(media_type, media_key)
        s3_key = make_s3_key(origin_file_name, media_key, created_at_ms or int(time.time() * 1000))

        if dry_run:
            log(f"WHATIF|{media_key}|{local_path}|{bucket}|{s3_key}")
            skip += 1
            continue

        # Live migration
        try:
            data = local_path.read_bytes()
            minio_client.put_object(
                bucket_name=bucket,
                object_name=s3_key,
                data=local_path.open("rb"),
                length=local_path.stat().st_size,
                content_type=mime or "application/octet-stream",
            )
            new_storage_path = s3_key
            cur.execute(
                f"UPDATE chat_media_asset SET storage_provider='s3', storage_path='{sanitize_key(new_storage_path)}' "
                f"WHERE media_id = {media_id}"
            )
            conn.commit()
            total_bytes += size_bytes or 0
            log(f"OK|{media_key}|{bucket}|{new_storage_path}|{size_bytes} bytes")
            ok += 1
        except Exception as e:
            log(f"FAIL|{media_key}|{e}")
            fail += 1

    # ---------------------------------------------------------------------------
    # Step 5: Migrate user.icon (avatar URLs pointing to local files)
    # ---------------------------------------------------------------------------
    print(f"\n--- Step 4: Checking user.icon for local avatar paths ---")
    log("-" * 60)
    log("Checking user.icon for local uploads...")

    cur.execute("""
        SELECT uid, icon FROM "user"
        WHERE icon IS NOT NULL
          AND icon != ''
          AND icon NOT LIKE 'http://%'
          AND icon NOT LIKE 'https://%'
          AND icon NOT LIKE 'qrc:/%'
          AND icon NOT LIKE ':/%'
    """)

    avatar_ok = avatar_skip = avatar_fail = 0

    for row in cur.fetchall():
        uid, icon_path = row
        local_path = resolve_local_path(icon_path, UPLOADS_ROOT)
        if local_path is None:
            log(f"AVATAR_SKIP|uid={uid}|no local file found at '{icon_path}'")
            avatar_skip += 1
            continue

        file_name = local_path.name
        s3_key = f"assets/avatar/{uid}_{file_name}"

        if dry_run:
            log(f"AVATAR_WHATIF|uid={uid}|{local_path}|memochat-avatar|{s3_key}")
            avatar_skip += 1
            continue

        try:
            minio_client.put_object(
                bucket_name="memochat-avatar",
                object_name=s3_key,
                data=local_path.open("rb"),
                length=local_path.stat().st_size,
            )
            new_icon_url = f"{MINIO_PUBLIC_URL}/memochat-avatar/{s3_key}"
            cur.execute(
                f"UPDATE \"user\" SET icon='{sanitize_key(new_icon_url)}' WHERE uid = {uid}"
            )
            conn.commit()
            log(f"AVATAR_OK|uid={uid}|{new_icon_url}")
            avatar_ok += 1
        except Exception as e:
            log(f"AVATAR_FAIL|uid={uid}|{e}")
            avatar_fail += 1

    # ---------------------------------------------------------------------------
    # Summary
    # ---------------------------------------------------------------------------
    print(f"\n{'='*60}")
    print(f"  Migration Complete  [{mode}]")
    print(f"{'='*60}\n")
    log("=" * 60)
    log("SUMMARY:")
    log(f"  Media records  : OK={ok}  SKIP={skip}  FAIL={fail}")
    log(f"  Avatar records : OK={avatar_ok}  SKIP={avatar_skip}  FAIL={avatar_fail}")
    mb = round(total_bytes / (1024 * 1024), 2)
    log(f"  Total bytes uploaded : {total_bytes} ({mb} MB)")

    print(f"Media records  : OK={ok}  SKIP={skip}  FAIL={fail}")
    print(f"Avatar records : OK={avatar_ok}  SKIP={avatar_skip}  FAIL={avatar_fail}")
    print(f"Total uploaded : {mb} MB")
    print(f"\nLog file: {log_file}")
    log("Migration finished.")

    if not dry_run and (fail > 0 or avatar_fail > 0):
        print(f"\nWARNING: Some records failed. Check log file.")
        print(f"Rollback: UPDATE FROM {backup_table}")

    cur.close()
    conn.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Migrate MemoChat media to MinIO")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview migration without making changes")
    args = parser.parse_args()
    run_migration(dry_run=args.dry_run)
