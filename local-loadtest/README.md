# MemoChat Load Test Toolkit

Location: `d:\MemoChat-Qml-Drogon\local-loadtest`

## Included
- Existing login load tests:
  - `http_login_load.py`
  - `tcp_login_load.py`
- New auth load tests:
  - `http_verify_code_load.py`
  - `http_register_load.py`
  - `http_reset_password_load.py`
- New business load tests:
  - `tcp_friendship_load.py`
  - `tcp_group_ops_load.py`
  - `tcp_history_ack_load.py`
  - `media_load.py`
  - `tcp_call_invite_load.py`
- New capacity load tests:
  - `postgresql_capacity_load.py`
  - `redis_capacity_load.py`
- Shared helpers:
  - `memochat_load_common.py`
  - `config.json`
  - `accounts.example.csv`
  - `run_suite.ps1`

## Prerequisites
- Python 3.8+
- Python packages: `psycopg[binary]`, `pymysql`, `redis`
- MemoChat services started:
  - `GateServer`
  - `StatusServer`
  - Configured `ChatServer` node set
  - `VarifyServer`
- Local PostgreSQL and Redis should match `config.json`

## Accounts
Recommended flow:

1. Copy `accounts.example.csv` to `accounts.local.csv`, or set `runtime_accounts_csv` in `config.json`.
2. Keep at least `2 * concurrency` accounts for friend and history tests.
3. Optional fields `user`, `uid`, `user_id`, `last_password`, `tags` will be auto-filled or refreshed by scripts.

Example CSV:

```csv
email,password,user,uid,user_id,last_password,tags
u1@example.com,pass1,load_user_1,,,,seed
u2@example.com,pass2,load_user_2,,,,seed
```

## Config
`config.json` now includes:
- shared gate and account settings
- mutable runtime account CSV path
- PostgreSQL and Redis direct connection settings
- per-scenario settings for auth, friendship, group, history, media, call, PostgreSQL, and Redis

The suite is allowed to mutate local dev PostgreSQL and Redis. It does not auto-clean data in v1.

## Run Individual Tests
Examples:

```powershell
python .\http_verify_code_load.py --config .\config.json
python .\http_register_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\http_reset_password_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\tcp_friendship_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\tcp_group_ops_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\tcp_history_ack_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\media_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\tcp_call_invite_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\postgresql_capacity_load.py --config .\config.json --accounts-csv .\accounts.local.csv
python .\redis_capacity_load.py --config .\config.json
```

## Run Suite
Supported scenarios:
- `auth`
- `login`
- `friend`
- `group`
- `history`
- `media`
- `call`
- `postgresql`
- `redis`
- `all`

Examples:

```powershell
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario auth
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario friend
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario all
```

`all` runs in this order:
1. auth
2. login
3. friendship
4. group ops
5. history and read ack
6. media upload and download
7. call invite
8. PostgreSQL capacity
9. Redis capacity

## Reports And Logs
- Runtime loadtest logs now default to `Memo_ops/artifacts/loadtest/runtime/logs/`
- Runtime loadtest reports now default to `Memo_ops/artifacts/loadtest/runtime/reports/`
- Historical ad-hoc and e2e samples are archived under `Memo_ops/artifacts/loadtest/history/`
- `run_suite.ps1` writes a timestamped suite folder and `suite_summary.json`
- Report schema includes:
  - `scenario`
  - `summary`
  - `phase_breakdown`
  - `top_errors`
  - `preconditions`
  - `data_mutation_summary`
- JSON line logs go to `logs/*.json`
- Log correlation fields include:
  - `trace_id`
  - `scenario`
  - `stage`
  - `account_email`
  - `group_id`
  - `peer_uid`
  - `upload_id`

## Notes
- Passwords can still use XOR encoding via `use_xor_passwd=true`
- Auth and business tests may create accounts, friendships, groups, messages, read state rows, and media assets
- PostgreSQL and Redis capacity tests are business-shaped synthetic workloads, not empty benchmark loops
- Chat prerequisites are now config-driven by `server/StatusServer/config.ini` `[Cluster]`, not fixed to exactly two chat nodes
