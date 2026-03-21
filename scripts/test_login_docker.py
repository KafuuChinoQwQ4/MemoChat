#!/usr/bin/env python3
"""Test login with an existing Docker account."""
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

# Check the uid=1001 account
    cur.execute('SELECT uid, email, pwd FROM "user" WHERE uid = 1002 LIMIT 1')
row = cur.fetchone()
if row:
    uid, email, pwd_stored = row
    print(f'uid=1001: email={email}, pwd_stored={repr(pwd_stored)}')
    decoded = xor_encode(pwd_stored)
    print(f'  decoded (plaintext)={repr(decoded)}')
    
    # Client sends XOR(plaintext) = XOR(xor_encode(pwd_stored)) = xor_encode(xor_encode(plaintext))
    # GateServer: Decode(stored) = xor_encode(stored) = xor_encode(xor_encode(plaintext)) = original plaintext
    # Compare: input == Decode(stored) => XOR(plaintext) == plaintext => FALSE
    client_sends = xor_encode(decoded)
    print(f'  Client sends XOR(decoded)={repr(client_sends)}')
    
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
            print(f'  Login: uid={data.get("uid")} error={data.get("error")}')
    except urllib.request.HTTPError as e:
        print(f'  Login HTTP Error: {e.code}')
    except Exception as ex:
        print(f'  Login Exception: {type(ex).__name__} {str(ex)[:100]}')
else:
    print('uid=1001 not found')

conn.close()
