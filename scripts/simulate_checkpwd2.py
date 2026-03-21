#!/usr/bin/env python3
"""Simulate GateServer CheckPwd with exact connection string from config.ini."""

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

# GateServer config.ini: Host=127.0.0.1 Port=5432 User=memochat Passwd=123456 Database=memo_pg
conn = psycopg.connect(
    host="127.0.0.1",
    port=5432,
    user="memochat",
    password="123456",
    dbname="memo_pg",
    autocommit=True
)
conn.autocommit = True
cur = conn.cursor()

test_accounts = [
    ('perf_test_1900000039@loadtest.local', '!a!CGC0ZXGoy', 1285),
    ('register_18064572i4vvh7h@loadtest.local', 'Pwd6FCrv24bv', 1278),
]

for email, plaintext, uid in test_accounts:
    print(f"\n=== uid={uid} email={email} ===")
    
    # Check with quoted 'user' table
    cur.execute('SELECT uid, pwd FROM "user" WHERE email = %s LIMIT 1', (email,))
    rows = cur.fetchall()
    if not rows:
        print(f"  user not found with quoted 'user' table")
        continue
    
    row = rows[0]
    db_uid, origin_pwd = row[0], row[1]
    print(f"  uid={db_uid} origin_pwd={repr(origin_pwd)} len={len(origin_pwd)}")
    
    decoded_pwd = decode_legacy_xor(origin_pwd)
    xor_sent = xor_encode(plaintext)
    
    print(f"  decoded={repr(decoded_pwd)} == plaintext={repr(plaintext)}? {decoded_pwd == plaintext}")
    print(f"  xor_sent={repr(xor_sent)} == origin_pwd={repr(origin_pwd)}? {xor_sent == origin_pwd}")

conn.close()
