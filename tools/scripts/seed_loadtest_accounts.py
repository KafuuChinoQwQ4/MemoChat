#!/usr/bin/env python3
"""
Seed 600 fresh load-test accounts directly into PostgreSQL.
Works for the Python load test tool and the MemoChat XOR-encoded login flow.

Passwords are stored as-is in the DB (the GateServer /user_login reads pwd from the DB and
compares with the XOR-encoded value the client sends — no double encoding).
"""
import psycopg2
import random
import string
from pathlib import Path
import sys

DB = dict(
    host="127.0.0.1",
    port=15432,
    user="memochat",
    password="123456",
    database="memo_pg",
)

def xor_encode(raw: str) -> str:
    """Mirror the GateServer DecodeLegacyXorPwd inverse: encode for DB storage."""
    x = len(raw) % 255
    return "".join(chr((ord(ch) ^ x) & 0xFF) for ch in raw)

def random_pwd(length=12):
    chars = string.ascii_letters + string.digits + "!@#$%^&*"
    return "".join(random.choices(chars, k=length))

def random_user_id():
    return "u" + str(random.randint(100_000_000, 999_999_999))

BATCH = int(sys.argv[1]) if len(sys.argv) > 1 else 2000
REPO_ROOT = Path(__file__).resolve().parents[2]

def main():
    print("[seed] Connecting to PostgreSQL...")
    conn = psycopg2.connect(**DB)
    conn.autocommit = True
    cur = conn.cursor()

    # Pick a unique uid range not colliding with existing users (max uid ≈ 1284)
    cur.execute("SELECT COALESCE(MAX(uid), 1000) FROM memo.user")
    max_uid = cur.fetchone()[0]
    start_uid = max_uid + 1
    print(f"[seed] Existing max uid={max_uid}, will insert uids {start_uid}–{start_uid + BATCH - 1}")

    # Reserve uid block in user_id table
    next_id = cur.execute(
        "SELECT COALESCE(MAX(id), 1000) + 1 FROM memo.user_id"
    )
    next_id = cur.fetchone()[0]
    for uid in range(start_uid, start_uid + BATCH):
        cur.execute("INSERT INTO memo.user_id(id) VALUES (%s) ON CONFLICT DO NOTHING", (next_id,))
        next_id += 1

    # Batch-insert users with XOR-encoded passwords
    ts = 1800000000
    rows = []
    csv_lines = ["email,password,user,uid,user_id,last_password,tags"]

    for i in range(BATCH):
        ts += random.randint(10, 99)
        name = f"perf_test_{ts}"
        email = f"{name}@loadtest.local"
        raw_pwd = random_pwd()
        xor_pwd = xor_encode(raw_pwd)
        uid = start_uid + i
        user_id = random_user_id()
        rows.append((uid, name, email, xor_pwd, name, ":/res/head_1.jpg", user_id))
        csv_lines.append(f"{email},{raw_pwd},{name},{uid},{user_id},,")

    # Bulk insert via executemany
    cur.executemany(
        "INSERT INTO memo.user(uid, name, email, pwd, nick, icon, user_id) "
        "VALUES (%s,%s,%s,%s,%s,%s,%s) "
        "ON CONFLICT (email) DO NOTHING",
        rows
    )
    inserted = cur.rowcount
    print(f"[seed] Inserted {inserted} accounts (duplicates skipped)")

    # Write CSV
    csv_path = REPO_ROOT / "infra" / "Memo_ops" / "artifacts" / "loadtest" / "runtime" / "accounts" / "accounts.local.csv"
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with open(csv_path, "w", encoding="utf-8") as f:
        f.write("\n".join(csv_lines))
    print(f"[seed] CSV written to {csv_path} ({BATCH} entries)")

    # Quick verification: try 3 random accounts
    sample = random.sample(rows, min(3, len(rows)))
    print("[seed] Sample verification (decoding stored pwd back):")
    for uid, name, email, xor_pwd, *_ in sample:
        decoded = xor_encode(xor_pwd)  # double-xor = original
        print(f"  uid={uid} email={email} stored_len={len(xor_pwd)} decoded={decoded}")

    cur.close()
    conn.close()
    print("[seed] Done.")

if __name__ == "__main__":
    main()
