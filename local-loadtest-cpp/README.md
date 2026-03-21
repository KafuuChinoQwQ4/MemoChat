# MemoChat Load Test Toolkit

**Location:** `local-loadtest-cpp/` (unified toolkit — formerly `local-loadtest/` and `local-loadtest-cpp/`)

A high-performance, multi-protocol load testing framework for MemoChat services, combining:

- **C++ native binaries** (TCP / QUIC / HTTP login) — `memochat_loadtest.exe`, powered by msquic + WinHTTP + Winsock2
- **Python business scenarios** (auth / friendship / group / history / media / call / capacity)
- **Protocol comparison tools** (TCP vs QUIC vs HTTP side-by-side)

```
local-loadtest-cpp/
├── config.json               # Unified config (C++ binary + Python scripts)
├── accounts.example.csv      # Example account CSV for C++ binary
├── run_suite.ps1             # Orchestrate all scenarios end-to-end
├── cpp_run.py                # C++ binary wrapper + report converter
├── compare_protocols.py      # TCP vs QUIC vs HTTP comparison report
├── run_protocol_comparison.py # One-shot protocol comparison runner
│
├── include/                  # C++ public headers
│   ├── config.hpp            # Config & account loading
│   ├── logger.hpp            # JSON logger (spdlog-based)
│   ├── metrics.hpp           # Latency stats, percentiles, report output
│   ├── monitor.hpp           # Real-time progress & system metrics
│   ├── client.hpp            # HTTP / TCP / QUIC client interfaces
│   ├── scenario.hpp          # Scenario strategy interface + factory
│   ├── runner.hpp            # Test runner (async task pool)
│   └── quic_lib.hpp          # msquic library singleton
│
├── src/
│   ├── core/                 # config.cpp, logger.cpp, metrics.cpp
│   ├── clients/              # client.cpp (WinHTTP+Winsock2), quic_client.cpp, quic_lib.cpp
│   ├── monitor/              # monitor.cpp (progress + system metrics)
│   ├── scenarios/            # tcp_scenario.cpp, quic_scenario.cpp, http_scenario.cpp
│   ├── runner.cpp            # Thread pool, warmup, result collection
│   └── main.cpp              # CLI entry point
│
├── CMakeLists.txt
└── README.md
```

---

## Quick Start

### 1. Build the C++ binary

Requires: Visual Studio 2022, CMake, vcpkg with `spdlog` installed.

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake `
    -DVCPKG_MANIFEST_MODE=ON

cmake --build build --config Release
```

### 2. Prepare accounts

```powershell
# Copy and edit accounts CSV
cp accounts.example.csv accounts.local.csv
# Add your test accounts (email,password columns required)
```

### 3. Run scenarios

```powershell
# One-shot TCP vs QUIC vs HTTP comparison
python run_protocol_comparison.py --total 500 --concurrency 50

# Direct C++ binary (TCP login)
.\build\Release\memochat_loadtest.exe --scenario tcp --total 500 --concurrency 50 --config config.json

# Direct C++ binary (QUIC login)
.\build\Release\memochat_loadtest.exe --scenario quic --filter-accounts --config config.json

# Direct C++ binary (HTTP login)
.\build\Release\memochat_loadtest.exe --scenario http --config config.json

# Full suite via PowerShell
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario all
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario login
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario auth
.\run_suite.ps1 -Config .\config.json -AccountsCsv .\accounts.local.csv -Scenario friend
```

---

## Architecture

### Two execution engines

| Scenario type | Engine | How to run |
|---|---|---|
| TCP / QUIC / HTTP login | C++ binary (`memochat_loadtest.exe`) | Direct or via `cpp_run.py` |
| Auth / Friendship / Group / History / Media / Call / Capacity | Python (`local-loadtest/*.py`) | Direct |

The PowerShell suite (`run_suite.ps1`) orchestrates both engines automatically.

### C++ binary (`memochat_loadtest.exe`)

Native Windows binaries with:
- **TCP** — Winsock2 persistent socket per request
- **QUIC** — Microsoft msquic (native, multi-threaded, 10k+ RPS)
- **HTTP** — WinHTTP (Gate-only, no persistent connection)
- Live CPU% / RSS monitoring in the terminal
- `--filter-accounts` probes each account via HTTP before the run
- JSON structured logs + JSON report with per-phase latency breakdown

### Python scripts (`../local-loadtest/*.py`)

Full business scenario coverage:
- `http_verify_code_load.py` — 验证码获取
- `http_register_load.py` — 注册
- `http_reset_password_load.py` — 重置密码
- `tcp_friendship_load.py` — 好友申请 / 认证
- `tcp_group_ops_load.py` — 建群 / 加群
- `tcp_history_ack_load.py` — 历史消息拉取 + 已读回执
- `media_load.py` — 文件上传 / 下载
- `tcp_call_invite_load.py` — 音视频呼叫邀请
- `postgresql_capacity_load.py` — PostgreSQL 容量测试
- `redis_capacity_load.py` — Redis 容量测试

---

## Config (`config.json`)

A single config file drives both the C++ binary and Python scripts.

### C++ binary fields

| Field | Description | Default |
|---|---|---|
| `gate_url` | GateServer HTTP endpoint | `http://127.0.0.1:8080` |
| `login_path` | Gate login path | `/user_login` |
| `client_ver` | Client version string | `2.0.0` |
| `use_xor_passwd` | XOR-encode password before sending | `true` |
| `accounts_csv` | Path to accounts CSV (required by C++ binary) | (required) |
| `total` | Total requests | `500` |
| `concurrency` | Concurrent async workers | `50` |
| `http_timeout_sec` | HTTP request timeout | `5.0` |
| `tcp_timeout_sec` | TCP socket timeout | `5.0` |
| `quic_timeout_sec` | QUIC handshake/response timeout | `5.0` |
| `protocol_version` | ChatServer protocol version | `3` |
| `quic_alpn` | QUIC ALPN string | `memochat-chat` |
| `warmup_iterations` | Warmup request count | `2` |
| `max_retries` | Retries per request | `2` |
| `log_dir` | JSON log output directory | (relative to config) |
| `report_dir` | JSON report output directory | (relative to config) |
| `log_level` | spdlog level: DEBUG\|INFO\|WARN\|ERROR | `INFO` |

### Scenario-specific overrides (C++ binary reads these)

| Section | Example fields |
|---|---|
| `http_login` | `total`, `concurrency`, `timeout_sec` |
| `tcp_login` | `protocol_version`, `total`, `http_timeout_sec`, `tcp_timeout_sec` |
| `quic_login` | `protocol_version`, `total`, `http_timeout_sec`, `quic_timeout_sec`, `alpn` |

### Python-specific fields (not read by C++ binary)

| Section | Used by |
|---|---|
| `mysql` | `postgresql_capacity_load.py`, all Python scripts |
| `redis` | `redis_capacity_load.py`, all Python scripts |
| `register_verify_reset` | `http_verify_code_load.py`, `http_register_load.py`, `http_reset_password_load.py` |
| `friendship` | `tcp_friendship_load.py` |
| `group_ops` | `tcp_group_ops_load.py` |
| `history_ack` | `tcp_history_ack_load.py` |
| `media` | `media_load.py` |
| `call_invite` | `tcp_call_invite_load.py` |
| `mysql_capacity` | `postgresql_capacity_load.py` |
| `redis_capacity` | `redis_capacity_load.py` |

---

## Python Wrapper Scripts

### `cpp_run.py` — C++ binary wrapper

Wraps the C++ binary, auto-detects the executable path, and converts the C++ JSON report to the Python-compatible format so `run_suite.ps1` can treat all reports uniformly.

```powershell
python cpp_run.py --scenario tcp --total 500 --concurrency 50 --config config.json
python cpp_run.py --scenario quic --filter-accounts --config config.json
python cpp_run.py --scenario http --report-path my_report.json --config config.json
python cpp_run.py --skip-if-no-binary   # Silent skip if binary not built yet
```

### `compare_protocols.py` — Protocol comparison

Reads TCP, QUIC, and HTTP report JSONs and produces a comparison report with speedup ratios.

```powershell
# From a directory of reports
python compare_protocols.py --input-dir ../Memo_ops/artifacts/loadtest/runtime/reports

# From specific files
python compare_protocols.py --tcp-report tcp_login_*.json --quic-report quic_login_*.json

# All three protocols
python compare_protocols.py --tcp-report tcp_*.json --quic-report quic_*.json --http-report http_*.json --output compare.json
```

### `run_protocol_comparison.py` — One-shot comparison runner

Runs TCP + QUIC + HTTP back-to-back via `cpp_run.py`, then calls `compare_protocols.py`.

```powershell
python run_protocol_comparison.py --total 1000 --concurrency 100
python run_protocol_comparison.py --skip-quic --filter-accounts
python run_protocol_comparison.py --compare-only   # Regenerate comparison from existing reports
```

---

## Accounts CSV Format

The C++ binary reads accounts from a CSV file with these columns:

```csv
email,password,uid,user_id,user
loadtest_0001@example.com,ChangeMe123!,,,,seed
loadtest_0002@example.com,ChangeMe123!,,,,seed
```

Columns `email` and `password` are required. Lines starting with `#` are skipped.

Python scripts use the same format for `runtime_accounts_csv`.

---

## Report Output

| Tool | Output |
|---|---|
| C++ binary | `logs/loadtest_<scenario>.json` (per-request JSON lines) + `reports/<scenario>_login_<ts>.json` |
| `cpp_run.py` | Same as C++ binary, plus a Python-compatible report at `--report-path` |
| `run_suite.ps1` | `Memo_ops/artifacts/loadtest/runtime/reports/suite_<ts>/suite_summary.json` + per-scenario reports |
| `compare_protocols.py` | `compare_protocols.json` (or custom path) |

### Report schema (Python-compatible, emitted by `cpp_run.py`)

```json
{
  "scenario": "login.tcp",
  "test": "login.tcp",
  "summary": {
    "total": 500,
    "success": 498,
    "failed": 2,
    "success_rate": 0.996,
    "throughput_rps": 40.5,
    "elapsed_sec": 12.345,
    "latency_ms": {"min": 8.2, "avg": 23.1, "p50": 19.5, "p95": 45.2, "p99": 98.3, "max": 312.0}
  },
  "phase_breakdown": {
    "http_gate":     {"avg": 12.1, "p95": 28.3},
    "tcp_handshake": {"avg":  5.3, "p95": 12.1},
    "chat_login":    {"avg":  3.8, "p95":  9.7}
  },
  "top_errors": [["gate_login_failed", 1], ["tcp_connect_failed", 1]]
}
```

---

## C++ Binary CLI Reference

```powershell
# Basic usage
.\build\Release\memochat_loadtest.exe --config config.json

# Scenario selection
--scenario <name>     tcp|quic|http  (default: tcp)

# Load parameters
--total <n>           Total requests (default: from config)
--concurrency <n>      Concurrent workers (default: from config)
--warmup <n>          Warmup iterations (default: from config)
--retries <n>         Max retries per request (default: from config)

# Account filtering
--filter-accounts     HTTP-probe each account and keep only valid ones

# Help
--help, -h            Show full CLI options
```

---

## Suite Scenarios (`run_suite.ps1`)

| Scenario | Steps | Engine |
|---|---|---|
| `auth` | verify_code → register → reset | Python |
| `login` | tcp_login → http_login | C++ binary |
| `tcp` | tcp_login | C++ binary |
| `quic` | quic_login | C++ binary |
| `http` | http_login | C++ binary |
| `friend` | friendship ops | Python |
| `group` | group ops | Python |
| `history` | history + ack | Python |
| `media` | upload + download | Python |
| `call` | call invite | Python |
| `postgresql` / `mysql` | PostgreSQL capacity | Python |
| `redis` | Redis capacity | Python |
| `all` | auth + login + friend + group + history + media + call + postgresql + redis | Both |

---

## Performance Comparison: Python (aioquic) vs C++ (msquic)

| Feature | Python (aioquic) | C++ (msquic) |
|---|---|---|
| QUIC | aioquic (async, GIL) | msquic (native, multi-threaded) |
| Build | `pip install` | CMake + vcpkg |
| Performance | Python GIL ceiling (~1k RPS) | Native threads (10k+ RPS) |
| Windows | Good | Excellent |
| Cross-platform | Yes (macOS/Linux) | Linux needs separate impl |
| Account filter | Manual scripting | Built-in `--filter-accounts` |
| System metrics | Not built-in | Live CPU%, RSS in console |
| Warmup | Via Python scripts | Built-in `--warmup` |

---

## Notes

- Passwords can still use XOR encoding via `use_xor_passwd=true`
- Auth and business tests may create accounts, friendships, groups, messages, read state rows, and media assets
- PostgreSQL and Redis capacity tests are business-shaped synthetic workloads, not empty benchmark loops
- Chat prerequisites are config-driven by `server/StatusServer/config.ini` `[Cluster]`, not fixed to exactly two chat nodes
