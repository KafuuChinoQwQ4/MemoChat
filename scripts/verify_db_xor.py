import psycopg

conn = psycopg.connect(host='127.0.0.1', port=5432, user='memochat', password='123456', dbname='memo_pg', autocommit=True)
cur = conn.cursor()

# Get 3 accounts from the CSV format (plaintext passwords)
csv_accounts = [
    ('perf_test_1900000047@loadtest.local', 'Pwd6FCrv24bv'),  # from CSV
]

# Check corresponding DB entries
for email, csv_pwd in csv_accounts:
    cur.execute('SELECT uid, pwd FROM memo.user WHERE email = %s LIMIT 1', (email,))
    row = cur.fetchone()
    if row:
        uid, db_pwd = row
        print(f'CSV: {email} pwd={csv_pwd}')
        print(f'DB:  uid={uid} pwd={repr(db_pwd)} len={len(db_pwd)}')

        # C++ XOR encode CSV pwd
        x = len(csv_pwd) % 255
        def xor_encode(s):
            return ''.join(chr((ord(c) ^ x) & 0xFF) for c in s)
        cpp_xor = xor_encode(csv_pwd)
        print(f'C++ XOR({x}): {repr(cpp_xor)}')
        print(f'DB pwd match: {cpp_xor == db_pwd}')

        # Decode DB pwd back
        x_db = len(db_pwd) % 255
        decoded = ''.join(chr((ord(c) ^ x_db) & 0xFF) for c in db_pwd)
        print(f'DB decoded({x_db}): {repr(decoded)}')
        print(f'CSV match: {decoded == csv_pwd}')
    else:
        print(f'Not found: {email}')

# Also check the account used in the Python script test
cur.execute('SELECT uid, email, pwd FROM memo.user WHERE email = %s LIMIT 1', ('perf_test_1900000047@loadtest.local',))
row = cur.fetchone()
if row:
    print(f'\nPython test account: uid={row[0]} email={row[1]} pwd={repr(row[2])}')
    # Python XOR
    csv_pwd2 = 'Pwd6FCrv24bv'
    x2 = len(csv_pwd2) % 255
    def xor_encode2(s):
        out = []
        for ch in s:
            out.append(chr(ord(ch) ^ x2))
        return ''.join(out)
    py_xor = xor_encode2(csv_pwd2)
    print(f'Python XOR({x2}): {repr(py_xor)}')
    print(f'Match DB: {py_xor == row[2]}')
