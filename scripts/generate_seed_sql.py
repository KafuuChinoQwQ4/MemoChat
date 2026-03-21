#!/usr/bin/env python3
"""Generate SQL INSERT statements for seed accounts."""
import json

rows = json.load(open(r'D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\loadtest\runtime\accounts\seed_rows.json'))

# Reserve uid block in user_id table
next_id = 1285
uid_sql = f"SELECT COALESCE(MAX(id), 1000) + 1 FROM memo.user_id"
print(f"-- next_id query: {uid_sql}")

# Generate user_id table inserts
uid_ins = [f"INSERT INTO memo.user_id(id) VALUES ({next_id + i});" for i in range(600)]
print("\n-- user_id reserved:")
for s in uid_ins[:5]:
    print(s)
print(f"  ... ({len(uid_ins)} total)")

# Generate user table inserts
batch_size = 100
for batch in range(6):
    start = batch * batch_size
    end = start + batch_size
    vals = []
    for r in rows[start:end]:
        uid = r['uid']
        name = r['name'].replace("'", "''")
        email = r['email'].replace("'", "''")
        # Escape backslash and single quote for E'...' notation
        xor_pwd = r['xor_pwd'].replace("\\", "\\\\").replace("'", "''")
        user_id = r['user_id']
        vals.append(f"({uid}, '{name}', '{email}', E'{xor_pwd}', '{name}', ':/res/head_1.jpg', '{user_id}')")
    sql = f"INSERT INTO memo.user(uid, name, email, pwd, nick, icon, user_id) VALUES\n    " + ",\n    ".join(vals) + "\nON CONFLICT(email) DO NOTHING;"
    fname = f"D:/MemoChat-Qml-Drogon/Memo_ops/artifacts/loadtest/runtime/accounts/insert_batch_{batch}.sql"
    with open(fname, 'w', encoding='utf-8') as f:
        f.write(sql)
    print(f"\n-- Batch {batch} SQL written to {fname} ({len(vals)} rows)")
    print(f"  First row: {vals[0][:120]}...")
