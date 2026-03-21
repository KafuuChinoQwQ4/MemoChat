#!/usr/bin/env python3
import random
import string
import os

def xor_encode(raw):
    x = len(raw) % 255
    return ''.join(chr((ord(c) ^ x) & 0xFF) for c in raw)

def rnd_pwd():
    chars = string.ascii_letters + string.digits + '!@#$%^&*'
    return ''.join(random.choices(chars, k=random.randint(11, 13)))

ts = 1900000000
rows = []
csv_lines = ['email,password,user,uid,user_id,last_password,tags']

for i in range(600):
    ts += random.randint(10, 99)
    name = f'perf_test_{ts}'
    email = f'{name}@loadtest.local'
    raw_pwd = rnd_pwd()
    xor_pwd = xor_encode(raw_pwd)
    uid = 1285 + i
    user_id = f'u{random.randint(100000000, 999999999)}'
    rows.append({
        'uid': uid,
        'name': name,
        'email': email,
        'pwd': raw_pwd,
        'xor_pwd': xor_pwd,
        'user_id': user_id,
    })
    csv_lines.append(f'{email},{raw_pwd},{name},{uid},{user_id},,')

# Write CSV
csv_path = r'D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\loadtest\runtime\accounts\accounts.local.csv'
with open(csv_path, 'w', encoding='utf-8') as f:
    f.write('\n'.join(csv_lines))
print(f'CSV written: {len(csv_lines)} lines to {csv_path}')

# Verify: double-xor should give back original
for r in rows[:3]:
    check = xor_encode(r['xor_pwd'])
    print(f"  uid={r['uid']} pwd_len={len(r['pwd'])} stored_len={len(r['xor_pwd'])} verify={check == r['pwd']}")

# Save rows to a temp JSON so we can use them
tmp_path = os.path.join(os.path.dirname(csv_path), 'seed_rows.json')
import json
with open(tmp_path, 'w', encoding='utf-8') as f:
    json.dump(rows, f)
print(f'Rows saved to {tmp_path}')
