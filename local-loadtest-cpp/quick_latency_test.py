#!/usr/bin/env python3
"""Quick latency test for optimized GateServer."""
import asyncio
import httpx
import time

async def test():
    c = httpx.AsyncClient(base_url="http://127.0.0.1:8080", timeout=10.0)
    body = {"email": "perf_test_1900000040@loadtest.local", "passwd": "4STodvR&OKaZ", "client_ver": "3.0.0"}
    
    print("Testing GateServer login latency (optimized):")
    print("=" * 60)
    
    for i in range(10):
        t0 = time.perf_counter()
        r = await c.post("/user_login", json=body)
        t1 = time.perf_counter()
        lat = (t1 - t0) * 1000
        d = r.json()
        
        if "stage_metrics" in d:
            sm = d["stage_metrics"]
            mysql_ms = sm.get("mysql_check_pwd_ms", 0)
            redis_ms = sm.get("redis_pipeline_ms", 0)
            rtt_count = sm.get("redis_rtt_count", 0)
            total = sm.get("user_login_total_ms", 0)
            print(f"  [{i+1:2d}] {lat:7.1f}ms | mysql={mysql_ms}ms redis={redis_ms}ms rtt={rtt_count} total={total}ms uid={d.get('uid')} cache_hit={sm.get('redis_pipeline_ms', 0) > 0}")
        else:
            print(f"  [{i+1:2d}] {lat:7.1f}ms | error={d.get('error')} uid={d.get('uid')}")
    
    await c.aclose()

asyncio.run(test())
