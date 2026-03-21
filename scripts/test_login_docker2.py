#!/usr/bin/env python3
import psycopg
import urllib.request
import json

def xor_encode(raw):
    x = len(raw) % 255
    return ''.join(chr((ord(c) ^ x) & 0xFF) for c in raw)

conn = psycopg.connect(
    host='127.0.0.1',
    port=5432,
    user='memochat',
    password='123456',
    dbname='memo_pg',
    autocommit=True,
    options='-csearch_path=memo'
)
cur = conn.cursor()

# Find first account
cur.execute('SELECT uid, email, pwd FROM "user" ORDER BY uid LIMIT 1')
row = cur.fetchone()
if row:
    uid, email, pwd_stored = row
    print(f'Found uid={uid} email={email}')
    decoded = xor_encode(pwd_stored)
    print(f'Plaintext={repr(decoded)}')
    client_sends = xor_encode(decoded)
    print(f'Client sends={repr(client_sends)}')
    body = json.dumps({'email': email, 'passwd': client_sends, 'client_ver': '2.0.0'}).encode()
    req = urllib.request.Request(
        'http://127.0.0.1:8080/user_login',
        data=body,
        headers={'Content-Type': 'application/json'},
        method='POST'
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            data = json.loads(resp.read().decode())
            print(f'Login SUCCESS: uid={data.get("uid")} error={data.get("error")}')
    except urllib.request.HTTPError as e:
        print(f'Login HTTP Error {e.code}')
    except Exception as ex:
        print(f'Login Exception: {type(ex).__name__} {str(ex)[:200]}')
else:
    print('No accounts found')

conn.close()
