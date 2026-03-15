import argparse
import json
import socket
import struct
import time
import urllib.request
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

from memochat_load_common import (
    build_summary,
    chat_login_payload,
    chat_protocol_version,
    filter_login_ready_accounts,
    finalize_report,
    get_log_dir,
    get_runtime_accounts_csv,
    init_json_logger,
    load_accounts,
    load_json,
    new_trace_id,
    relation_bootstrap_mode,
    save_accounts_csv,
    top_errors,
    utc_now_str,
    xor_encode,
)

CHAT_LOGIN_REQ = 1005
CHAT_LOGIN_RSP = 1006


def recv_exact(sock: socket.socket, n: int) -> bytes:
    out = bytearray()
    while len(out) < n:
        chunk = sock.recv(n - len(out))
        if not chunk:
            break
        out.extend(chunk)
    return bytes(out)


def http_login(gate_url: str, login_path: str, email: str, password: str, client_ver: str,
               timeout: float, use_xor: bool, trace_id: str):
    passwd = xor_encode(password) if use_xor else password
    body = json.dumps({'email': email, 'passwd': passwd, 'client_ver': client_ver}, separators=(',', ':')).encode('utf-8')
    req = urllib.request.Request(
        f'{gate_url}{login_path}',
        data=body,
        headers={'Content-Type': 'application/json', 'X-Trace-Id': trace_id},
        method='POST',
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        raw = resp.read().decode('utf-8', errors='replace')
        payload = json.loads(raw)
        return int(getattr(resp, 'status', 0) or 0), payload


def main() -> int:
    parser = argparse.ArgumentParser(description='MemoChat TCP chat-login load test (HTTP login + TCP 1005 handshake)')
    parser.add_argument('--config', default='config.json', help='Path to config.json')
    parser.add_argument('--accounts-csv', default='', help='CSV path with header: email,password')
    parser.add_argument('--report-path', default='', help='Explicit report output path')
    parser.add_argument('--total', type=int, default=0)
    parser.add_argument('--concurrency', type=int, default=0)
    parser.add_argument('--http-timeout', type=float, default=0)
    parser.add_argument('--tcp-timeout', type=float, default=0)
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger('tcp_login_loadtest', log_dir=get_log_dir(cfg))
    tcp_cfg = cfg.get('tcp_login', {})
    total = args.total if args.total > 0 else int(tcp_cfg.get('total', 1000))
    concurrency = args.concurrency if args.concurrency > 0 else int(tcp_cfg.get('concurrency', 100))
    http_timeout = args.http_timeout if args.http_timeout > 0 else float(tcp_cfg.get('http_timeout_sec', 5))
    tcp_timeout = args.tcp_timeout if args.tcp_timeout > 0 else float(tcp_cfg.get('tcp_timeout_sec', 5))

    gate_url = cfg.get('gate_url', 'http://127.0.0.1:8080').rstrip('/')
    login_path = cfg.get('login_path', '/user_login')
    client_ver = cfg.get('client_ver', '2.0.0')
    use_xor = bool(cfg.get('use_xor_passwd', True))
    login_protocol_version = chat_protocol_version(cfg)

    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)
    accounts = load_accounts(cfg, runtime_csv)
    filtered_accounts = filter_login_ready_accounts(cfg, accounts, http_timeout, logger)
    if filtered_accounts:
        accounts = filtered_accounts
        if runtime_csv:
            save_accounts_csv(runtime_csv, accounts)
    else:
        logger.warning('login probe produced no ready accounts, fallback to raw runtime accounts',
                       extra={'event': 'loadtest.accounts.login_probe_empty_fallback',
                              'payload': {'raw_accounts': len(accounts)}})
    if not accounts:
        logger.error('No accounts found. Fill config.json accounts or provide --accounts-csv',
                     extra={'event': 'loadtest.input.missing_accounts'})
        return 2

    if len(accounts) < concurrency:
        logger.warning('accounts less than concurrency, token race may increase failures',
                       extra={
                           'event': 'loadtest.tcp_login.account_race_risk',
                           'payload': {'accounts': len(accounts), 'concurrency': concurrency},
                       })

    latencies = []
    success = 0
    failed = 0
    error_counter = Counter()

    def one(i: int):
        acc = accounts[i % len(accounts)]
        trace_id = new_trace_id()
        t0 = time.perf_counter()
        stage = 'unknown'
        ok = False
        try:
            code, login_payload = http_login(
                gate_url, login_path, acc['email'], acc['password'], client_ver, http_timeout, use_xor, trace_id
            )
            if code != 200:
                stage = f'http_status_{code}'
                return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id

            if int(login_payload.get('error', -1)) != 0:
                stage = f"http_error_{login_payload.get('error')}"
                return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id

            uid = int(login_payload.get('uid', 0))
            token = str(login_payload.get('token', ''))
            login_ticket = str(login_payload.get('login_ticket', ''))
            host = str(login_payload.get('host', '')).strip() or '127.0.0.1'
            port = int(str(login_payload.get('port', '0')).strip() or '0')
            if uid <= 0 or port <= 0:
                stage = 'http_payload_invalid'
                return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id
            if login_protocol_version >= 3 and not login_ticket:
                stage = 'http_missing_login_ticket'
                return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id
            if login_protocol_version < 3 and not token:
                stage = 'http_missing_token'
                return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id

            payload = json.dumps(chat_login_payload(cfg, login_payload, trace_id), separators=(',', ':')).encode('utf-8')
            packet = struct.pack('!HH', CHAT_LOGIN_REQ, len(payload)) + payload

            with socket.create_connection((host, port), timeout=tcp_timeout) as s:
                s.settimeout(tcp_timeout)
                s.sendall(packet)
                header = recv_exact(s, 4)
                if len(header) != 4:
                    stage = 'tcp_no_header'
                    return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id

                msg_id, msg_len = struct.unpack('!HH', header)
                body = recv_exact(s, msg_len)
                if len(body) != msg_len:
                    stage = 'tcp_incomplete_body'
                    return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id

                if msg_id != CHAT_LOGIN_RSP:
                    stage = f'tcp_unexpected_msg_{msg_id}'
                    return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id

                rsp = json.loads(body.decode('utf-8', errors='replace'))
                if int(rsp.get('error', -1)) == 0:
                    ok = True
                    stage = 'ok'
                else:
                    stage = f"tcp_error_{rsp.get('error')}"
        except Exception as e:  # noqa: BLE001
            stage = f'exception_{type(e).__name__}'

        return ok, stage, (time.perf_counter() - t0) * 1000.0, trace_id

    begin = time.perf_counter()
    with ThreadPoolExecutor(max_workers=concurrency) as ex:
        for ok, stage, elapsed, trace_id in ex.map(one, range(total)):
            latencies.append(elapsed)
            if ok:
                success += 1
            else:
                failed += 1
                error_counter[stage] += 1
                logger.warning('tcp login request failed',
                               extra={
                                   'event': 'loadtest.tcp_login.failed',
                                   'trace_id': trace_id,
                                   'payload': {'stage': stage},
                               })
    elapsed_sec = time.perf_counter() - begin

    summary = build_summary(latencies, success, failed, elapsed_sec)
    report = {
        'scenario': 'login.tcp',
        'test': 'tcp_login',
        'time_utc': utc_now_str(),
        'target': {'gate_url': gate_url, 'login_path': login_path},
        'config': {
            'total': total,
            'concurrency': concurrency,
            'http_timeout_sec': http_timeout,
            'tcp_timeout_sec': tcp_timeout,
            'accounts': len(accounts),
            'use_xor_passwd': use_xor,
            'login_protocol_version': login_protocol_version,
            'relation_bootstrap_mode': relation_bootstrap_mode(cfg),
        },
        'summary': summary,
        'error_counter': dict(error_counter),
        'top_errors': top_errors(error_counter),
        'phase_breakdown': {'tcp_login_chain': summary['latency_ms']},
        'chat_login_stage_breakdown': {
            'ticket_verify_ms': {},
            'session_attach_ms': {},
            'relation_bootstrap_ms': {},
        },
        'preconditions': {'service': ['GateServer', 'StatusServer', 'ConfiguredChatNodes', 'Redis']},
        'data_mutation_summary': {'tcp_login_requests': success},
    }

    report_path = finalize_report('tcp_login', report, args.report_path, cfg)

    logger.info('tcp login load test completed',
                extra={
                    'event': 'loadtest.tcp_login.summary',
                    'scenario': 'login.tcp',
                    'payload': {
                        'target': {'gate_url': gate_url, 'login_path': login_path},
                        'total': total,
                        'concurrency': concurrency,
                        'accounts': len(accounts),
                        'login_protocol_version': login_protocol_version,
                        'relation_bootstrap_mode': relation_bootstrap_mode(cfg),
                        'success': summary['success'],
                        'failed': summary['failed'],
                        'success_rate': summary['success_rate'],
                        'rps': summary['throughput_rps'],
                        'p95': summary['latency_ms']['p95'],
                        'p99': summary['latency_ms']['p99'],
                        'top_errors': top_errors(error_counter),
                        'report': report_path,
                    },
                })
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
