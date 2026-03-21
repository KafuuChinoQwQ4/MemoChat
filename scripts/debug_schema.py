#!/usr/bin/env python3
"""Debug PostgreSQL schema and table visibility."""
import psycopg

conn = psycopg.connect(
    host='127.0.0.1',
    port=5432,
    user='memochat',
    password='123456',
    dbname='memo_pg',
    autocommit=True
)
cur = conn.cursor()

# Check tables in each schema
for schema in ['public', 'memo', 'pg_catalog']:
    cur.execute(
        "SELECT table_name FROM information_schema.tables WHERE table_schema = %s ORDER BY table_name",
        (schema,)
    )
    rows = cur.fetchall()
    print(f"\nSchema '{schema}' tables: {[r[0] for r in rows]}")

# Test: can we see memo.user columns?
cur.execute(
    "SELECT column_name, data_type FROM information_schema.columns WHERE table_schema = %s AND table_name = %s ORDER BY ordinal_position",
    ('memo', 'user')
)
print("\nmemo.user columns:")
for r in cur.fetchall():
    print(f"  {r[0]}: {r[1]}")

# Test: raw SELECT from memo.user (no schema prefix)
print("\nTesting bare 'user' table:")
try:
    cur.execute("SELECT uid FROM user LIMIT 1")
    print("  OK: bare 'user' table exists")
except Exception as e:
    print(f"  ERROR: {e}")

# Test: full path
print("\nTesting 'memo.user':")
try:
    cur.execute("SELECT uid FROM memo.user LIMIT 1")
    print("  OK: memo.user exists")
except Exception as e:
    print(f"  ERROR: {e}")

# Check search_path
cur.execute("SHOW search_path")
print("\nDefault search_path:", cur.fetchone()[0])

conn.close()
