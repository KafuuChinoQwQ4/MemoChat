import psycopg
import json

conn = psycopg.connect(
    host="127.0.0.1",
    port=5432,
    user="memochat",
    password="123456",
    dbname="memo_pg",
    options="-csearch_path=memo,public",
    autocommit=True
)

cur = conn.cursor()

# Check the first test account (perf_test_1900000039)
cur.execute("""
    SELECT uid, email, pwd, length(pwd) as pwd_len
    FROM "user"
    WHERE email LIKE 'perf_test_1900000039%%'
    LIMIT 1
""")
rows = cur.fetchall()
print("=== perf_test account ===")
for r in rows:
    print(f"  uid={r[0]}, email={r[1]}, pwd={r[2]!r}, len={r[3]}")
    # Also show the XOR decode of it
    pwd_raw = r[2]
    x = len(pwd_raw) % 255
    decoded = ''.join(chr(ord(c) ^ x) for c in pwd_raw)
    print(f"  XOR(len={len(pwd_raw)}, code={x}) decode: {decoded!r}")

# Check a sample of early accounts (e.g., uid 1001-1010)
cur.execute("""
    SELECT uid, email, pwd, length(pwd) as pwd_len
    FROM "user"
    WHERE uid >= 1001 AND uid <= 1010
    ORDER BY uid
    LIMIT 10
""")
rows = cur.fetchall()
print("\n=== Sample accounts (uid 1001-1010) ===")
for r in rows:
    pwd_raw = r[2]
    x = len(pwd_raw) % 255
    decoded = ''.join(chr(ord(c) ^ x) for c in pwd_raw)
    print(f"  uid={r[0]}, email={r[1]}, pwd_len={r[3]}, XOR_decode={decoded!r}")

# Check total count
cur.execute("SELECT COUNT(*) FROM \"user\"")
cnt = cur.fetchone()[0]
print(f"\nTotal users in memo.user: {cnt}")

# Check how many are perf_test
cur.execute("SELECT COUNT(*) FROM \"user\" WHERE email LIKE 'perf_test%%'")
cnt_perf = cur.fetchone()[0]
print(f"Total perf_test users: {cnt_perf}")

# Check how many are in the user_id table
cur.execute("SELECT COUNT(*) FROM user_id")
cnt_id = cur.fetchone()[0]
print(f"Total entries in user_id: {cnt_id}")

cur.close()
conn.close()
