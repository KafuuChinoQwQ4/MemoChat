#!/usr/bin/env python3
"""Seed 600 fresh load-test accounts into Docker PostgreSQL (memo.user table)."""
import json
import psycopg

# Connect to Docker PostgreSQL (mapped from container to localhost:5432)
DB = dict(
    host="127.0.0.1",
    port=5432,
    user="memochat",
    password="123456",
    dbname="memo_pg",
    options="-c search_path=memo,public",
    autocommit=True,
)

def xor_encode(raw: str) -> str:
    """Mirror GateServer DecodeLegacyXorPwd inverse."""
    x = len(raw) % 255
    return "".join(chr((ord(c) ^ x) & 0xFF) for c in raw)

def main():
    seed_path = r"D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\loadtest\runtime\accounts\seed_rows.json"
    rows = json.load(open(seed_path, encoding="utf-8"))
    print(f"[seed] Loaded {len(rows)} account rows")

    conn = psycopg.connect(**DB)
    cur = conn.cursor()

    # Check current state
    cur.execute("SELECT COUNT(*) FROM \"user\"")
    print(f"[seed] Current user count: {cur.fetchone()[0]}")

    # Reserve uid block
    uid_start = 1285
    uid_end = 1884
    print(f"[seed] Reserving uid block {uid_start}-{uid_end} in user_id table...")
    for uid in range(uid_start, uid_end + 1):
        cur.execute("INSERT INTO user_id(id) VALUES (%s) ON CONFLICT DO NOTHING", (uid,))
    print("[seed] uid block reserved.")

    # Batch-insert users
    batch_size = 100
    total_inserted = 0
    for batch_idx in range(6):
        start = batch_idx * batch_size
        end = start + batch_size
        batch = rows[start:end]
        values = []
        for r in batch:
            xor_pwd = xor_encode(r["pwd"])
            values.append((
                r["uid"],
                r["name"],
                r["email"],
                xor_pwd,
                r["name"],
                ":/res/head_1.jpg",
                r["user_id"],
            ))

        cur.executemany(
            "INSERT INTO \"user\"(uid, name, email, pwd, nick, icon, user_id) "
            "VALUES (%s, %s, %s, %s, %s, %s, %s) "
            "ON CONFLICT(email) DO NOTHING",
            values
        )
        inserted = cur.rowcount
        total_inserted += inserted
        print(f"[seed] Batch {batch_idx} ({start+1}-{end}): {inserted} inserted")

    print(f"[seed] Total inserted: {total_inserted}")

    # Verify
    cur.execute("SELECT COUNT(*) FROM \"user\" WHERE email LIKE %s", ("%perf_test%",))
    count = cur.fetchone()[0]
    print(f"[seed] perf_test accounts now: {count}")

    # Sample check
    print("[seed] Sample verification:")
    for r in rows[:3]:
        cur.execute("SELECT pwd FROM \"user\" WHERE uid = %s LIMIT 1", (r["uid"],))
        row = cur.fetchone()
        if row:
            decoded = xor_encode(row[0])
            ok = "OK" if decoded == r["pwd"] else "MISMATCH"
            print(f"  uid={r['uid']} pwd_len={len(r['pwd'])} decoded={decoded} [{ok}]")

    cur.close()
    conn.close()
    print("[seed] Done.")

if __name__ == "__main__":
    main()
