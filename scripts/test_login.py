import subprocess, urllib.request, json

# Query PostgreSQL
cmd = ['docker', 'exec', 'memochat-postgres', 'psql', '-U', 'memochat', '-d', 'memo_pg', '-t', '-c', "SELECT uid, email, pwd FROM memo.user WHERE uid=1001"]
result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
print('SQL output:', repr(result.stdout))

line = result.stdout.strip()
parts = [p.strip() for p in line.split('|')]
uid = int(parts[0])
email = parts[1].strip("'")
pwd_encoded = parts[2]
print('uid=', uid, 'email=', email, 'pwd_enc=', repr(pwd_encoded))

# Decode XOR
key = len(pwd_encoded) % 255
if key == 0:
    key = len(pwd_encoded)
plaintext = ''.join(chr(ord(c) ^ key) for c in pwd_encoded)
print('plaintext:', plaintext, 'len=', len(plaintext))

# Login test
payload = json.dumps({'email': email, 'passwd': plaintext, 'client_ver': '2.0.0'}).encode()
req = urllib.request.Request('http://127.0.0.1:8080/user_login', data=payload, headers={'Content-Type': 'application/json'})
try:
    with urllib.request.urlopen(req, timeout=10) as r:
        data = json.loads(r.read())
        print('Result:', 'error=' + str(data.get('error')), 'uid=' + str(data.get('uid')))
except Exception as e:
    print('EXCEPTION:', str(e))
