#!/usr/bin/env python3
"""Simulate exactly what GateServer PostgresDao::CheckPwd does."""
import psycopg

def xor_encode(raw):
    x = len(raw) % 255
    return ''.join(chr((ord(c) ^ x) & 0xFF) for c in raw)

def decode_legacy_xor(input_str):
    x = len(input_str) % 255
    decoded = ''
    for i in range(len(input_str)):
        decoded += chr(ord(input_str[i]) ^ x)
    return decoded

# Connect using the exact connection string GateServer would use
conn = psycopg.connect(
    host="127.0.0.1",
    port=5432,
    user="memochat",
    password="123456",
    dbname="memo_pg"
)
conn.autocommit = True
cur = conn.cursor()

test_accounts = [
    ('register_18064572i4vvh7h@loadtest.local', 'Pwd6FCrv24bv'),  # uid=1278
    ('perf_test_1900000039@loadtest.local', '!a!CGC0ZXGoy'),     # uid=1285
]

for email, plaintext in test_accounts:
    print(f"\n=== Testing {email} ===")
    # Step 1: SELECT
    cur.execute(
        'SELECT uid, name, email, pwd FROM "user" WHERE email = %s LIMIT 1',
        (email,)
    )
    rows = cur.fetchall()
    if not rows:
        print(f"  FAIL: user not found")
        continue
    
    row = rows[0]
    uid, name, db_email, origin_pwd = row[0], row[1], row[2], row[3]
    print(f"  uid={uid}, email={db_email}")
    print(f"  origin_pwd repr={repr(origin_pwd)}, len={len(origin_pwd)}")
    
    # Step 2: Decode
    decoded_pwd = decode_legacy_xor(origin_pwd)
    print(f"  decoded_pwd={repr(decoded_pwd)}, len={len(decoded_pwd)}")
    
    # Step 3: Compare
    xor_sent = xor_encode(plaintext)
    print(f"  client XOR-sends={repr(xor_sent)}, len={len(xor_sent)}")
    
    if xor_sent == origin_pwd:
        print(f"  [PASS] XOR(client_plaintext) == DB stored")
    else:
        print(f"  [FAIL] XOR(client_plaintext) != DB stored")
    
    if decoded_pwd == plaintext:
        print(f"  [PASS] Decode(DB stored) == plaintext")
    else:
        print(f"  [FAIL] Decode(DB stored) != plaintext")

conn.close()
