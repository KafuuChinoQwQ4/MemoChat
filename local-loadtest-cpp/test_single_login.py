#!/usr/bin/env python3
"""Single login test."""
import asyncio, sys, time
sys.path.insert(0, '.')
from tcp_quic_latency_test import http_gate_login_async, close_http_client

async def test():
    print("Testing GateServer login latency (optimized):")
    print("=" * 60)
    for i in range(10):
        t0 = time.perf_counter()
        d = await http_gate_login_async("perf_test_1900000040@loadtest.local", "4STodvR&OKaZ", timeout=30.0)
        t1 = time.perf_counter()
        lat = (t1 - t0) * 1000
        print(f"[{i+1:2d}] {lat:7.1f}ms | error={d.get('error')} uid={d.get('uid')}")
        if "stage_metrics" in d:
            sm = d["stage_metrics"]
            print(f"     mysql={sm.get('mysql_check_pwd_ms')}ms redis={sm.get('redis_pipeline_ms')}ms rtt={sm.get('redis_rtt_count')} total={sm.get('user_login_total_ms')}ms cache_hit={sm.get('mysql_check_pwd_ms', -1) == 0}")
    await close_http_client()

asyncio.run(test())
