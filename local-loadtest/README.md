# MemoChat Load Test Toolkit

Location: `D:\MemoChat-LoadTest`

## Included
- `http_login_load.py`: GateServer `/user_login` pressure test
- `tcp_login_load.py`: full chain pressure test (HTTP login -> TCP chat login 1005/1006)
- `run_suite.ps1`: one-click runner
- `config.json`: default configuration
- `accounts.example.csv`: account file template
- `reports/`: JSON reports output

## Prerequisites
- Python 3.8+
- MemoChat services started (`GateServer`, `StatusServer`, `ChatServer`, `ChatServer2`, `VarifyServer`)

## 1) Prepare accounts
Option A: edit `config.json` -> `accounts`

Option B (recommended): create `accounts.csv` with header:

```csv
email,password
u1@example.com,pass1
u2@example.com,pass2
```

> For stable TCP results, account count should be >= concurrency, otherwise token race may increase failures.

## 2) Run tests
### HTTP login
```powershell
cd D:\MemoChat-LoadTest
python .\http_login_load.py --config .\config.json --accounts-csv .\accounts.csv
```

### TCP login chain
```powershell
cd D:\MemoChat-LoadTest
python .\tcp_login_load.py --config .\config.json --accounts-csv .\accounts.csv
```

### One-click suite
```powershell
cd D:\MemoChat-LoadTest
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.csv
```

## 3) Optional overrides
```powershell
python .\http_login_load.py --config .\config.json --accounts-csv .\accounts.csv --total 20000 --concurrency 500 --timeout 8
python .\tcp_login_load.py --config .\config.json --accounts-csv .\accounts.csv --total 5000 --concurrency 200 --http-timeout 8 --tcp-timeout 8
```

## Report format
JSON report saved in `reports/`, includes:
- success/fail and success rate
- throughput (RPS)
- latency stats (`p50/p90/p95/p99`)
- error counters

## Runtime logs
- JSON line logs are written to `logs/http_login_loadtest.json` and `logs/tcp_login_loadtest.json`.
- Every request carries `X-Trace-Id`; TCP login payload also includes optional `trace_id`.

## Notes
- Password can be XOR-encoded automatically (`use_xor_passwd=true` in config).
- GateServer in this project also accepts plain password fallback, but keeping XOR mode matches client behavior.
