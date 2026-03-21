#!/usr/bin/env python3
"""Seed 600 fresh load-test accounts into PostgreSQL using psycopg3."""
import json
import psycopg

DB = dict(
    host="127.0.0.1",
    port=5432,
    user="memochat",
    password="123456",
    dbname="memo_pg",
    autocommit=True,
)

def xor_encode(raw: str) -> str:
    """Mirror GateServer DecodeLegacyXorPwd inverse."""
    x = len(raw) % 255
    return "".join(chr((ord(c) ^ x) & 0xFF) for c in raw)

def main():
    # Load generated account data
    csv_path = r"D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\loadtest\runtime\accounts\accounts.local.csv"
    seed_path = r"D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\loadtest\runtime\accounts\seed_rows.json"
    rows = json.load(open(seed_path, encoding="utf-8"))
    print(f"[seed] Loaded {len(rows)} account rows from seed_rows.json")

    conn = psycopg.connect(**DB)
    cur = conn.cursor()

    # Reserve uid block in user_id table
    uid_start = 1285
    uid_end = 1884
    print(f"[seed] Reserving uid block {uid_start}-{uid_end} in user_id table...")
    for uid in range(uid_start, uid_end + 1):
        cur.execute("INSERT INTO memo.user_id(id) VALUES (%s) ON CONFLICT DO NOTHING", (uid,))
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
                r["name"],           # nick
                ":/res/head_1.jpg",  # icon
                r["user_id"],
            ))

        cur.executemany(
            "INSERT INTO memo.user(uid, name, email, pwd, nick, icon, user_id) "
            "VALUES (%s, %s, %s, %s, %s, %s, %s) "
            "ON CONFLICT(email) DO NOTHING",
            values
        )
        inserted = cur.rowcount
        total_inserted += inserted
        print(f"[seed] Batch {batch_idx} ({start+1}-{end}): {inserted} inserted")

    print(f"[seed] Total inserted: {total_inserted}")

    # Verify: fetch a few and check pwd decoding
    print("[seed] Verification:")
    for r in rows[:3]:
        uid = r["uid"]
        cur.execute("SELECT pwd FROM memo.user WHERE uid = %s LIMIT 1", (uid,))
        row = cur.fetchone()
        if row:
            stored = row[0]
            decoded = xor_encode(stored)
            ok = "OK" if decoded == r["pwd"] else "MISMATCH"
            print(f"  uid={uid} email={r['email']} stored_len={len(stored)} decoded={decoded} [{ok}]")

    cur.close()
    conn.close()
    print("[seed] Done.")

if __name__ == "__main__":
    main()
