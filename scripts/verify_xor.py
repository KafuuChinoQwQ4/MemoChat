def xor_encode(raw):
    x = len(raw) % 255
    return ''.join(chr((ord(c) ^ x) & 0xFF) for c in raw)

csv_pwd = 'Pwd6FCrv24bv'
xor_pwd = xor_encode(csv_pwd)
decoded = xor_encode(xor_pwd)
print('CSV pwd:', csv_pwd, 'len=', len(csv_pwd))
print('XOR pwd:', repr(xor_pwd), 'len=', len(xor_pwd))
print('Decoded:', decoded)
print('Match:', decoded == csv_pwd)
