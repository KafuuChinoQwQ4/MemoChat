import argparse
import json
import time
import urllib.error
import urllib.request
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

from memochat_load_common import (
    build_summary,
    filter_login_ready_accounts,
    finalize_report,
    get_log_dir,
    get_runtime_accounts_csv,
    init_json_logger,
    load_accounts,
    load_json,
    new_trace_id,
    save_accounts_csv,
    top_errors,
    utc_now_str,
    xor_encode,
)


def main() -> int:
    parser = argparse.ArgumentParser(description='MemoChat GateServer /user_login load test')
    parser.add_argument('--config', default='config.json', help='Path to config.json')
    parser.add_argument('--accounts-csv', default='', help='CSV path with header: email,password')
    parser.add_argument('--report-path', default='', help='Explicit report output path')
    parser.add_argument('--total', type=int, default=0, help='Override total requests')
    parser.add_argument('--concurrency', type=int, default=0, help='Override concurrency')
    parser.add_argument('--timeout', type=float, default=0, help='Override HTTP timeout seconds')
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger('http_login_loadtest', log_dir=get_log_dir(cfg))
    http_cfg = cfg.get('http_login', {})
    total = args.total if args.total > 0 else int(http_cfg.get('total', 1000))
    concurrency = args.concurrency if args.concurrency > 0 else int(http_cfg.get('concurrency', 100))
    timeout = args.timeout if args.timeout > 0 else float(http_cfg.get('timeout_sec', 5))

    gate_url = cfg.get('gate_url', 'http://127.0.0.1:8080').rstrip('/')
    login_path = cfg.get('login_path', '/user_login')
    client_ver = cfg.get('client_ver', '2.0.0')
    use_xor = bool(cfg.get('use_xor_passwd', True))

    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)
    accounts = load_accounts(cfg, runtime_csv)
    filtered_accounts = filter_login_ready_accounts(cfg, accounts, timeout, logger)
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

    url = f'{gate_url}{login_path}'
    status_counter = Counter()
    error_counter = Counter()
    latencies = []
    success = 0
    failed = 0

    def one(i: int):
        acc = accounts[i % len(accounts)]
        trace_id = new_trace_id()
        passwd = xor_encode(acc['password']) if use_xor else acc['password']
        body = json.dumps({
            'email': acc['email'],
            'passwd': passwd,
            'client_ver': client_ver,
        }, separators=(',', ':')).encode('utf-8')
        headers = {'Content-Type': 'application/json', 'X-Trace-Id': trace_id}
        req = urllib.request.Request(url, data=body, headers=headers, method='POST')

        t0 = time.perf_counter()
        ok = False
        stage = 'unknown'
        code = 0
        try:
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                code = int(getattr(resp, 'status', 0) or 0)
                raw = resp.read().decode('utf-8', errors='replace')
                payload = json.loads(raw)
                server_error = int(payload.get('error', -1))
                ok = (code == 200 and server_error == 0)
                stage = f'http_{code}_err_{server_error}'
        except urllib.error.HTTPError as e:
            code = int(getattr(e, 'code', 0) or 0)
            stage = f'http_error_{code}'
        except Exception as e:  # noqa: BLE001
            stage = f'exception_{type(e).__name__}'

        elapsed_ms = (time.perf_counter() - t0) * 1000.0
        return ok, stage, code, elapsed_ms, trace_id

    begin = time.perf_counter()
    with ThreadPoolExecutor(max_workers=concurrency) as ex:
        for ok, stage, code, elapsed, trace_id in ex.map(one, range(total)):
            latencies.append(elapsed)
            status_counter[str(code)] += 1
            if ok:
                success += 1
            else:
                failed += 1
                error_counter[stage] += 1
                logger.warning('request failed',
                               extra={
                                   'event': 'loadtest.http_login.failed',
                                   'trace_id': trace_id,
                                   'payload': {'stage': stage, 'http_status': code},
                               })
    elapsed_sec = time.perf_counter() - begin

    summary = build_summary(latencies, success, failed, elapsed_sec)
    report = {
        'scenario': 'login.http',
        'test': 'http_login',
        'time_utc': utc_now_str(),
        'target': url,
        'config': {
            'total': total,
            'concurrency': concurrency,
            'timeout_sec': timeout,
            'accounts': len(accounts),
            'use_xor_passwd': use_xor,
        },
        'summary': summary,
        'phase_breakdown': {'http_login': summary['latency_ms']},
        'http_status_counter': dict(status_counter),
        'error_counter': dict(error_counter),
        'top_errors': top_errors(error_counter),
        'preconditions': {'service': ['GateServer', 'StatusServer', 'Redis']},
        'data_mutation_summary': {'login_requests': success},
    }

    report_path = finalize_report('http_login', report, args.report_path, cfg)

    logger.info('http login load test completed',
                extra={
                    'event': 'loadtest.http_login.summary',
                    'scenario': 'login.http',
                    'payload': {
                        'target': url,
                        'total': total,
                        'concurrency': concurrency,
                        'accounts': len(accounts),
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
