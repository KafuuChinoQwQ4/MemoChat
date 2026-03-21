#!/usr/bin/env python3
"""Check which PostgreSQL instance has the perf_test accounts."""
import psycopg

# Docker PostgreSQL (without search_path - uses default public schema)
print("=== Docker PostgreSQL (no search_path) ===")
try:
    conn = psycopg.connect(host='127.0.0.1', port=5432, user='memochat', password='123456',
                           dbname='memo_pg', autocommit=True)
    cur = conn.cursor()
    cur.execute('SELECT COUNT(*) FROM "user"')
    total = cur.fetchone()[0]
    print(f"  \"user\" table count: {total}")
    conn.close()
except Exception as e:
    print(f"  Error: {e}")

# Docker PostgreSQL with search_path=memo
print("\n=== Docker PostgreSQL (search_path=memo) ===")
try:
    conn = psycopg.connect(host='127.0.0.1', port=5432, user='memochat', password='123456',
                           dbname='memo_pg', autocommit=True, options='-csearch_path=memo')
    cur = conn.cursor()
    cur.execute('SELECT COUNT(*) FROM "user"')
    total = cur.fetchone()[0]
    print(f"  \"user\" table count: {total}")
    cur.execute('SELECT uid FROM "user" ORDER BY uid DESC LIMIT 5')
    print(f"  max uid: {cur.fetchall()}")
    conn.close()
except Exception as e:
    print(f"  Error: {e}")

# Windows local PostgreSQL
print("\n=== Windows Local PostgreSQL ===")
try:
    conn2 = psycopg.connect(host='127.0.0.1', port=5432, user='memochat', password='123456',
                            dbname='memo_pg', autocommit=True)
    cur2 = conn2.cursor()
    cur2.execute('SELECT COUNT(*) FROM "user"')
    total = cur2.fetchone()[0]
    print(f"  \"user\" table count: {total}")
    cur2.execute('SELECT uid FROM "user" ORDER BY uid DESC LIMIT 5')
    print(f"  max uid: {cur2.fetchall()}")
    conn2.close()
except Exception as e:
    print(f"  Error connecting to local: {e}")
