import argparse
import os
import time
from urllib.parse import urlencode

from memochat_load_common import (
    connect_chat_client,
    ensure_accounts,
    ensure_temp_file,
    finalize_report,
    gate_api_url,
    get_log_dir,
    get_runtime_accounts_csv,
    http_binary_request,
    http_download,
    http_json_request,
    init_json_logger,
    load_json,
    refresh_account_profiles,
    run_parallel,
    sha256_file,
    utc_now_str,
)


def main() -> int:
    parser = argparse.ArgumentParser(description="MemoChat media upload/download load test")
    parser.add_argument("--config", default="config.json", help="Path to config.json")
    parser.add_argument("--accounts-csv", default="", help="Runtime accounts csv path")
    parser.add_argument("--report-path", default="", help="Explicit report output path")
    parser.add_argument("--total", type=int, default=0, help="Override total flows")
    parser.add_argument("--concurrency", type=int, default=0, help="Override concurrency")
    parser.add_argument("--http-timeout", type=float, default=0, help="Override HTTP timeout seconds")
    args = parser.parse_args()

    cfg = load_json(args.config)
    logger = init_json_logger("media_loadtest", log_dir=get_log_dir(cfg))
    test_cfg = cfg.get("media", {})
    total = args.total if args.total > 0 else int(test_cfg.get("total", 10))
    concurrency = args.concurrency if args.concurrency > 0 else int(test_cfg.get("concurrency", 2))
    http_timeout = args.http_timeout if args.http_timeout > 0 else float(test_cfg.get("http_timeout_sec", 30))
    runtime_csv = get_runtime_accounts_csv(cfg, args.accounts_csv)
    size_kb_list = list(test_cfg.get("file_sizes_kb", [128, 512]))
    media_types = list(test_cfg.get("media_types", ["image", "file"]))

    accounts = ensure_accounts(cfg, max(total, concurrency), runtime_csv, logger, "media")
    accounts = refresh_account_profiles(cfg, accounts)
    total = min(total, len(accounts))
    accounts = accounts[:total]

    def worker(i: int):
        account = accounts[i]
        file_size = int(size_kb_list[i % len(size_kb_list)]) * 1024
        media_type = str(media_types[i % len(media_types)])
        suffix = ".jpg" if media_type == "image" else ".bin"
        local_path = ensure_temp_file(file_size, suffix)
        local_hash = sha256_file(local_path)
        client = None
        upload_id = ""
        phase_ms = {}
        ok = False
        stage = "unknown"
        started = time.perf_counter()
        try:
            client, account = connect_chat_client(cfg, account, http_timeout, http_timeout, f"media-{i}", f"media-{i}")
            uid = int(account["uid"])
            token = account["token"]
            file_name = os.path.basename(local_path)

            t0 = time.perf_counter()
            _, init_rsp = http_json_request(
                gate_api_url(cfg, "/upload_media_init"),
                payload={
                    "uid": uid,
                    "token": token,
                    "media_type": media_type,
                    "file_name": file_name,
                    "mime": "image/jpeg" if media_type == "image" else "application/octet-stream",
                    "file_size": file_size,
                    "chunk_size": 256 * 1024,
                },
                timeout=http_timeout,
                method="POST",
            )
            phase_ms["upload_init"] = (time.perf_counter() - t0) * 1000.0
            if int(init_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"upload init failed: {init_rsp}")
            upload_id = str(init_rsp.get("upload_id", ""))
            chunk_size = int(init_rsp.get("chunk_size", 256 * 1024))

            t1 = time.perf_counter()
            with open(local_path, "rb") as f:
                chunk_index = 0
                while True:
                    chunk = f.read(chunk_size)
                    if not chunk:
                        break
                    _, chunk_rsp = http_binary_request(
                        gate_api_url(cfg, "/upload_media_chunk"),
                        chunk,
                        headers={
                            "X-Uid": str(uid),
                            "X-Token": token,
                            "X-Upload-Id": upload_id,
                            "X-Chunk-Index": str(chunk_index),
                        },
                        timeout=http_timeout,
                    )
                    if int(chunk_rsp.get("error", -1)) != 0:
                        raise RuntimeError(f"upload chunk failed: {chunk_rsp}")
                    chunk_index += 1
            phase_ms["upload_chunk"] = (time.perf_counter() - t1) * 1000.0

            t2 = time.perf_counter()
            _, status_rsp = http_json_request(
                gate_api_url(cfg, f"/upload_media_status?{urlencode({'uid': uid, 'token': token, 'upload_id': upload_id})}"),
                payload=None,
                timeout=http_timeout,
                method="GET",
            )
            phase_ms["upload_status"] = (time.perf_counter() - t2) * 1000.0
            if int(status_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"upload status failed: {status_rsp}")

            t3 = time.perf_counter()
            _, complete_rsp = http_json_request(
                gate_api_url(cfg, "/upload_media_complete"),
                payload={"uid": uid, "token": token, "upload_id": upload_id},
                timeout=http_timeout,
                method="POST",
            )
            phase_ms["upload_complete"] = (time.perf_counter() - t3) * 1000.0
            if int(complete_rsp.get("error", -1)) != 0:
                raise RuntimeError(f"upload complete failed: {complete_rsp}")

            download_url = str(complete_rsp.get("url", ""))
            if download_url.startswith("/"):
                download_url = gate_api_url(cfg, download_url)
            if "uid=" not in download_url:
                joiner = "&" if "?" in download_url else "?"
                download_url = f"{download_url}{joiner}{urlencode({'uid': uid, 'token': token})}"

            t4 = time.perf_counter()
            download_status, body, _ = http_download(download_url, http_timeout)
            phase_ms["download"] = (time.perf_counter() - t4) * 1000.0
            if download_status != 200:
                raise RuntimeError(f"download failed: http {download_status}")
            ok = (len(body) == file_size)
            if ok:
                import hashlib

                ok = hashlib.sha256(body).hexdigest() == local_hash
            stage = "ok" if ok else "hash_mismatch"
        except Exception as exc:  # noqa: BLE001
            stage = f"exception_{type(exc).__name__}"
            logger.warning(
                "media flow failed",
                extra={
                    "event": "loadtest.media.failed",
                    "scenario": "media",
                    "stage": stage,
                    "trace_id": f"media-{i}",
                    "account_email": account.get("email", ""),
                    "upload_id": upload_id,
                    "payload": {"error": str(exc), "media_type": media_type, "file_size": file_size},
                },
            )
        finally:
            if client:
                client.close()
            try:
                os.remove(local_path)
            except OSError:
                pass
        return {
            "ok": ok,
            "stage": stage,
            "elapsed_ms": (time.perf_counter() - started) * 1000.0,
            "phase_ms": phase_ms,
            "mutation": {"media_uploaded": 1 if ok else 0, "media_downloaded": 1 if ok else 0},
            "sample": {"account_email": account.get("email", ""), "upload_id": upload_id, "media_type": media_type},
        }

    result = run_parallel(total, min(concurrency, max(1, total)), worker)
    report = {
        "scenario": "media",
        "test": "media_load",
        "time_utc": utc_now_str(),
        "target": {
            "http": [
                "/upload_media_init",
                "/upload_media_chunk",
                "/upload_media_status",
                "/upload_media_complete",
                "/media/download",
            ]
        },
        "config": {
            "total": total,
            "concurrency": min(concurrency, max(1, total)),
            "http_timeout_sec": http_timeout,
            "runtime_accounts_csv": runtime_csv,
            "file_sizes_kb": size_kb_list,
            "media_types": media_types,
        },
        "summary": result["summary"],
        "phase_breakdown": result["phase_breakdown"],
        "error_counter": result["error_counter"],
        "top_errors": result["top_errors"],
        "preconditions": {"accounts": total, "service": ["GateServer", "MySQL", "Redis"]},
        "data_mutation_summary": result["data_mutation_summary"],
        "samples": result["samples"],
    }
    report_path = finalize_report("media_load", report, args.report_path, cfg)

    logger.info(
        "media load test completed",
        extra={
            "event": "loadtest.media.summary",
            "scenario": "media",
            "payload": {
                "total": total,
                "concurrency": min(concurrency, max(1, total)),
                "success": result["summary"]["success"],
                "failed": result["summary"]["failed"],
                "success_rate": result["summary"]["success_rate"],
                "rps": result["summary"]["throughput_rps"],
                "p95": result["summary"]["latency_ms"]["p95"],
                "p99": result["summary"]["latency_ms"]["p99"],
                "top_errors": result["top_errors"],
                "report": report_path,
            },
        },
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
