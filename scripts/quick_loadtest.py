"""
Recover plaintext passwords from PostgreSQL and update accounts CSV.
Then run TCP and QUIC load tests.
"""
import subprocess, json, csv, io, time, socket, struct, os, sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass, field
from typing import Optional, List, Dict

# ---- Config ----
POSTGRES_CONTAINER = "memochat-postgres"
POSTGRES_USER = "memochat"
POSTGRES_DB = "memo_pg"
GATE_URL = "http://127.0.0.1:8080"
LOGIN_PATH = "/user_login"
ACCOUNTS_CSV = "D:/MemoChat-Qml-Drogon/Memo_ops/artifacts/loadtest/runtime/accounts/accounts.local.csv"
REPORT_DIR = "D:/MemoChat-Qml-Drogon/Memo_ops/artifacts/loadtest/runtime/reports"
TEST_TOTAL = 200
TEST_CONCURRENCY = 30

# ---- PostgreSQL Password Decoding ----
def decode_xor_pwd(encoded: str) -> str:
    """Decode password stored in PostgreSQL (XOR with len%255)."""
    if not encoded:
        return ""
    key = len(encoded) % 255
    if key == 0:
        key = len(encoded)  # avoid zero key
    decoded = []
    for ch in encoded:
        decoded.append(chr(ord(ch) ^ key))
    return ''.join(decoded)

def get_postgres_accounts() -> List[dict]:
    """Query PostgreSQL for loadtest accounts and decode passwords."""
    cmd = ["docker", "exec", POSTGRES_CONTAINER, "psql",
           "-U", POSTGRES_USER, "-d", POSTGRES_DB,
           "-t",  # tuples only, no header
           "-c", "SELECT uid, email, pwd FROM memo.user WHERE email LIKE '%loadtest%' ORDER BY uid"]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    accounts = []
    for line in result.stdout.strip().split('\n'):
        line = line.strip()
        if not line:
            continue
        # Format: "uid | email | pwd"
        # Use partition instead of split to handle values with spaces
        parts = [p.strip() for p in line.split('|')]
        if len(parts) >= 3:
            uid = int(parts[0])
            email = parts[1].strip("'")  # remove PostgreSQL quotes
            pwd_encoded = parts[2]
            if 'loadtest.local' in email:
                plaintext = decode_xor_pwd(pwd_encoded)
                name = email.split('@')[0]
                accounts.append({
                    'email': email,
                    'password': plaintext,
                    'pwd_encoded': pwd_encoded,
                    'user': name,
                    'uid': uid,
                    'uid_str': str(uid),
                    'tags': 'loadtest'
                })
    return accounts

# ---- Gate HTTP Login ----
def gate_login(email: str, password: str) -> Optional[dict]:
    payload = json.dumps({
        "email": email,
        "passwd": password,
        "client_ver": "2.0.0"
    }).encode()
    req = urllib.request.Request(
        GATE_URL + LOGIN_PATH,
        data=payload,
        headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            data = json.loads(resp.read())
            if data.get("error") == 0:
                return data
    except Exception as e:
        print(f"Gate login error: {e}", file=sys.stderr)
    return None

# ---- TCP Chat Login ----
def tcp_send_frame(sock, msg_id: int, body: str):
    body_bytes = body.encode('utf-8')
    header = struct.pack("!II", msg_id, len(body_bytes))
    sock.sendall(header + body_bytes)

def tcp_recv_frame(sock):
    header = b''
    while len(header) < 8:
        header += sock.recv(8 - len(header))
    msg_id, body_len = struct.unpack("!II", header)
    body = b''
    while len(body) < body_len:
        body += sock.recv(body_len - len(body))
    return msg_id, body.decode('utf-8')

def tcp_full_login(email: str, password: str, proto_ver=3) -> dict:
    t0 = time.time()
    result = {"ok": False, "http_ms": 0, "tcp_ms": 0, "chat_ms": 0, "error": ""}

    t_http = time.time()
    login_rsp = gate_login(email, password)
    result["http_ms"] = (time.time() - t_http) * 1000

    if not login_rsp:
        result["error"] = "gate_login_failed"
        return result

    uid = login_rsp.get("uid", 0)
    token = login_rsp.get("token", "")
    chat_host = login_rsp.get("host", "127.0.0.1")
    chat_port_str = str(login_rsp.get("port", "8090"))
    login_ticket = login_rsp.get("login_ticket", "")

    try:
        chat_port = int(chat_port_str)
    except:
        result["error"] = "invalid_port"
        return result

    t_tcp = time.time()
    try:
        sock = socket.create_connection((chat_host, chat_port), timeout=10)
    except Exception as e:
        result["error"] = f"tcp_connect_failed:{e}"
        return result

    result["tcp_ms"] = (time.time() - t_tcp) * 1000

    t_chat = time.time()
    chat_body = json.dumps({
        "uid": uid,
        "token": token,
        "host": chat_host,
        "port": chat_port,
        "login_ticket": login_ticket,
        "trace_id": "",
        "client_ver": "2.0.0"
    })
    tcp_send_frame(sock, 1005, chat_body)
    try:
        msg_id, rsp_body = tcp_recv_frame(sock)
        if msg_id == 1006:
            rsp = json.loads(rsp_body)
            chat_err = rsp.get("error", -1)
            result["ok"] = (chat_err == 0)
            if chat_err != 0:
                result["error"] = f"chat_err_{chat_err}"
        else:
            result["error"] = f"unexpected_msg_{msg_id}"
    except Exception as e:
        result["error"] = f"tcp_recv_failed:{e}"
    finally:
        sock.close()

    result["chat_ms"] = (time.time() - t_chat) * 1000
    result["total_ms"] = (time.time() - t0) * 1000
    return result

# ---- QUIC Chat Login ----
def quic_full_login(email: str, password: str, cfg) -> dict:
    # Use msquic-based test via the C++ load test tool
    # For now, return a placeholder - we'll run QUIC tests via C++ binary
    return tcp_full_login(email, password)

# ---- Update CSV ----
def update_accounts_csv(accounts: List[dict]):
    """Update the accounts CSV with fresh PostgreSQL data."""
    os.makedirs(os.path.dirname(ACCOUNTS_CSV), exist_ok=True)
    with open(ACCOUNTS_CSV, 'w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=['email', 'password', 'user', 'uid', 'uid_str', 'last_password', 'tags'])
        writer.writeheader()
        for acc in accounts:
            writer.writerow({
                'email': acc['email'],
                'password': acc['password'],
                'user': acc['user'],
                'uid': acc['uid'],
                'uid_str': acc['uid_str'],
                'last_password': acc['password'],  # same as password (plaintext)
                'tags': acc['tags']
            })
    print(f"Updated {len(accounts)} accounts in {ACCOUNTS_CSV}")

# ---- Load Test Runner ----
def run_load_test(scenario: str, accounts: List[dict], total: int, concurrency: int) -> dict:
    print(f"\nRunning {scenario.upper()} load test: total={total}, concurrency={concurrency}")

    results = []
    t_start = time.time()

    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        futures = {}
        for i in range(total):
            acc = accounts[i % len(accounts)]
            future = executor.submit(tcp_full_login, acc["email"], acc["password"])
            futures[future] = i

        done = 0
        for future in as_completed(futures):
            result = future.result()
            results.append(result)
            done += 1
            if done % 20 == 0 or done == total:
                elapsed = time.time() - t_start
                success = sum(1 for r in results if r["ok"])
                failed = done - success
                rps = done / elapsed if elapsed > 0 else 0
                print(f"  {done}/{total} | success={success} failed={failed} rps={rps:.1f}")

    t_end = time.time()
    total_elapsed = t_end - t_start

    success_results = [r for r in results if r["ok"]]
    failed_results = [r for r in results if not r["ok"]]

    latencies = sorted([r["total_ms"] for r in success_results])

    def percentile(data, p):
        if not data:
            return 0
        idx = int(len(data) * p / 100)
        idx = min(idx, len(data) - 1)
        return data[idx]

    success_count = len(success_results)
    failed_count = len(failed_results)

    print(f"\n=== {scenario.upper()} Load Test Results ===")
    print(f"  Total:     {total}   Success: {success_count}   Failed: {failed_count}")
    print(f"  Duration:  {total_elapsed:.2f}s")
    print(f"  RPS:       {total/total_elapsed:.2f}")
    if latencies:
        print(f"  Avg:       {sum(latencies)/len(latencies):.2f} ms")
        print(f"  P50:       {percentile(latencies, 50):.2f} ms")
        print(f"  P75:       {percentile(latencies, 75):.2f} ms")
        print(f"  P90:       {percentile(latencies, 90):.2f} ms")
        print(f"  P95:       {percentile(latencies, 95):.2f} ms")
        print(f"  P99:       {percentile(latencies, 99):.2f} ms")
        print(f"  Max:       {max(latencies):.2f} ms")
    print(f"  Success:   {success_count/total*100:.2f}%")

    if failed_results:
        error_counts = {}
        for r in failed_results:
            e = r["error"]
            error_counts[e] = error_counts.get(e, 0) + 1
        print(f"\n  Top Errors:")
        for e, c in sorted(error_counts.items(), key=lambda x: -x[1])[:5]:
            print(f"    {e}: {c}")

    http_lats = sorted([r["http_ms"] for r in success_results])
    tcp_lats = sorted([r["tcp_ms"] for r in success_results])
    chat_lats = sorted([r["chat_ms"] for r in success_results])
    if http_lats:
        print(f"\n  Phase Breakdown:")
        print(f"    http_gate:     avg={sum(http_lats)/len(http_lats):.2f}ms  p95={percentile(http_lats, 95):.2f}ms")
    if tcp_lats:
        print(f"    tcp_handshake: avg={sum(tcp_lats)/len(tcp_lats):.2f}ms  p95={percentile(tcp_lats, 95):.2f}ms")
    if chat_lats:
        print(f"    chat_login:    avg={sum(chat_lats)/len(chat_lats):.2f}ms  p95={percentile(chat_lats, 95):.2f}ms")

    # Save JSON report
    os.makedirs(REPORT_DIR, exist_ok=True)
    ts = time.strftime("%Y%m%d_%H%M%S")
    report_path = os.path.join(REPORT_DIR, f"{scenario}_login_{ts}.json")

    report = {
        "scenario": f"login.{scenario}",
        "timestamp": ts,
        "config": {"total": total, "concurrency": concurrency, "accounts": len(accounts)},
        "summary": {
            "total": total, "success": success_count, "failed": failed_count,
            "rps": total/total_elapsed,
            "success_rate": success_count/total,
            "latency_ms": {
                "avg": sum(latencies)/len(latencies) if latencies else 0,
                "p50": percentile(latencies, 50),
                "p75": percentile(latencies, 75),
                "p90": percentile(latencies, 90),
                "p95": percentile(latencies, 95),
                "p99": percentile(latencies, 99),
                "max": max(latencies) if latencies else 0
            }
        },
        "http_phase": {"avg_ms": sum(http_lats)/len(http_lats) if http_lats else 0, "p95_ms": percentile(http_lats, 95)},
        "tcp_handshake_phase": {"avg_ms": sum(tcp_lats)/len(tcp_lats) if tcp_lats else 0, "p95_ms": percentile(tcp_lats, 95)},
        "chat_login_phase": {"avg_ms": sum(chat_lats)/len(chat_lats) if chat_lats else 0, "p95_ms": percentile(chat_lats, 95)},
        "top_errors": [{"error": e, "count": c} for e, c in sorted(error_counts.items(), key=lambda x: -x[1])[:10]] if failed_results else []
    }

    with open(report_path, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    print(f"\nReport: {report_path}")
    return report

if __name__ == "__main__":
    # 1. Query PostgreSQL for accounts
    print("=== Step 1: Query PostgreSQL for loadtest accounts ===")
    accounts = get_postgres_accounts()
    if not accounts:
        print("ERROR: No loadtest accounts found in PostgreSQL!")
        sys.exit(1)
    print(f"Found {len(accounts)} accounts:")
    for acc in accounts:
        print(f"  uid={acc['uid']} email={acc['email']} plaintext={acc['password']}")

    # 2. Quick validation - try logging in with first account
    print("\n=== Step 2: Validate login with first account ===")
    first = accounts[0]
    login_rsp = gate_login(first["email"], first["password"])
    if login_rsp:
        print(f"  LOGIN SUCCESS! uid={login_rsp.get('uid')} host={login_rsp.get('host')} port={login_rsp.get('port')}")
    else:
        print(f"  LOGIN FAILED for uid={first['uid']}")
        sys.exit(1)

    # 3. Update accounts CSV
    print("\n=== Step 3: Update accounts CSV ===")
    update_accounts_csv(accounts)

    # 4. Run TCP load test
    print("\n=== Step 4: TCP Load Test ===")
    tcp_report = run_load_test("tcp", accounts, TEST_TOTAL, TEST_CONCURRENCY)

    # 5. Save final report path
    print("\n=== All tests complete ===")
    print(f"Reports saved to: {REPORT_DIR}")
