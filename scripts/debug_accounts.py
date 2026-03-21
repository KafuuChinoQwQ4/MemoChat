#!/usr/bin/env python3
"""Thoroughly investigate where the 860 accounts are stored."""
import psycopg

conn = psycopg.connect(
    host='127.0.0.1',
    port=5432,
    user='memochat',
    password='123456',
    dbname='memo_pg',
    autocommit=True
)
conn2 = psycopg.connect(
    host='127.0.0.1',
    port=5432,
    user='memochat',
    password='123456',
    dbname='memo_pg',
    autocommit=True,
    options='-csearch_path=memo,public'
)
cur = conn.cursor()
cur2 = conn2.cursor()

print("=== Connection 1: No search_path ===")
print(f"  Connection 1 encoding: {conn.info.encoding}")
print(f"  Connection 1 server version: {conn.info.server_version}")

print("\n=== Connection 2: search_path=memo,public ===")

# Get all schemas
print("\n[All schemas]")
cur.execute("SELECT schema_name FROM information_schema.schemata WHERE schema_name NOT IN ('pg_catalog','information_schema') ORDER BY schema_name")
schemas = [r[0] for r in cur.fetchall()]
print(f"  Schemas: {schemas}")

# Check each schema for user tables
for schema in schemas:
    for conn_to_use, name in [(cur, "conn1"), (cur2, "conn2")]:
        try:
            conn_to_use.execute(f'SELECT COUNT(*) FROM "{schema}".user')
            count = conn_to_use.fetchone()[0]
            if count > 0:
                conn_to_use.execute(f'SELECT uid FROM "{schema}".user ORDER BY uid LIMIT 1')
                min_uid = conn_to_use.fetchone()[0]
                conn_to_use.execute(f'SELECT uid FROM "{schema}".user ORDER BY uid DESC LIMIT 1')
                max_uid = conn_to_use.fetchone()[0]
                print(f"  {name} {schema}.user: {count} rows (uid {min_uid} to {max_uid})")
        except Exception as e:
            pass  # Table doesn't exist in this schema

conn.close()
conn2.close()
