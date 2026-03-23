"""
MemoChat HTTP 压测脚本
测试 HTTP/1.1 登录性能和延迟
"""
import asyncio
import aiohttp
import time
import json
import statistics
from dataclasses import dataclass, field
from typing import List


@dataclass
class RequestResult:
    success: bool
    latency_ms: float
    error: str = ""


@dataclass
class BenchmarkResult:
    name: str
    total: int
    success: int
    failed: int
    latencies: List[float]
    
    @property
    def success_rate(self) -> float:
        return self.success / self.total * 100 if self.total > 0 else 0
    
    @property
    def avg_latency(self) -> float:
        return statistics.mean(self.latencies) if self.latencies else 0
    
    @property
    def min_latency(self) -> float:
        return min(self.latencies) if self.latencies else 0
    
    @property
    def max_latency(self) -> float:
        return max(self.latencies) if self.latencies else 0
    
    def percentile(self, p: float) -> float:
        if not self.latencies:
            return 0
        sorted_latencies = sorted(self.latencies)
        idx = int(len(sorted_latencies) * p / 100)
        idx = min(idx, len(sorted_latencies) - 1)
        return sorted_latencies[idx]
    
    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "total": self.total,
            "success": self.success,
            "failed": self.failed,
            "success_rate": f"{self.success_rate:.2f}%",
            "latency_ms": {
                "avg": f"{self.avg_latency:.2f}",
                "min": f"{self.min_latency:.2f}",
                "max": f"{self.max_latency:.2f}",
                "p50": f"{self.percentile(50):.2f}",
                "p90": f"{self.percentile(90):.2f}",
                "p98": f"{self.percentile(98):.2f}",
                "p99": f"{self.percentile(99):.2f}",
            }
        }


async def http_login(session: aiohttp.ClientSession, email: str, password: str) -> RequestResult:
    """执行 HTTP 登录请求"""
    start = time.perf_counter()
    try:
        async with session.post(
            "http://127.0.0.1:8080/user_login",
            json={"email": email, "passwd": password, "client_ver": "2.0.0"},
            timeout=aiohttp.ClientTimeout(total=5)
        ) as resp:
            data = await resp.json()
            latency = (time.perf_counter() - start) * 1000
            if resp.status == 200 and data.get("error") == 0:
                return RequestResult(success=True, latency_ms=latency)
            else:
                return RequestResult(success=False, latency_ms=latency, error=data.get("error", "unknown"))
    except Exception as e:
        latency = (time.perf_counter() - start) * 1000
        return RequestResult(success=False, latency_ms=latency, error=str(e))


async def benchmark_http_login(total: int, concurrency: int, accounts: List[tuple]) -> BenchmarkResult:
    """HTTP 登录压测"""
    results: List[RequestResult] = []
    semaphore = asyncio.Semaphore(concurrency)
    
    async def bounded_login(session: aiohttp.ClientSession, idx: int):
        async with semaphore:
            email, password = accounts[idx % len(accounts)]
            return await http_login(session, email, password)
    
    connector = aiohttp.TCPConnector(limit=concurrency, limit_per_host=concurrency)
    async with aiohttp.ClientSession(connector=connector) as session:
        tasks = [bounded_login(session, i) for i in range(total)]
        results = await asyncio.gather(*tasks)
    
    successes = [r for r in results if r.success]
    failures = [r for r in results if not r.success]
    
    return BenchmarkResult(
        name="HTTP/1.1 Login",
        total=total,
        success=len(successes),
        failed=len(failures),
        latencies=[r.latency_ms for r in successes]
    )


async def benchmark_http_auth(total: int, concurrency: int, accounts: List[tuple]) -> BenchmarkResult:
    """HTTP 获取验证码压测"""
    results: List[RequestResult] = []
    semaphore = asyncio.Semaphore(concurrency)
    
    async def bounded_verify(session: aiohttp.ClientSession, idx: int):
        async with semaphore:
            email, _ = accounts[idx % len(accounts)]
            start = time.perf_counter()
            try:
                async with session.post(
                    "http://127.0.0.1:8080/get_varifycode",
                    json={"email": email},
                    timeout=aiohttp.ClientTimeout(total=5)
                ) as resp:
                    latency = (time.perf_counter() - start) * 1000
                    if resp.status == 200:
                        return RequestResult(success=True, latency_ms=latency)
                    else:
                        return RequestResult(success=False, latency_ms=latency)
            except Exception as e:
                latency = (time.perf_counter() - start) * 1000
                return RequestResult(success=False, latency_ms=latency, error=str(e))
    
    connector = aiohttp.TCPConnector(limit=concurrency, limit_per_host=concurrency)
    async with aiohttp.ClientSession(connector=connector) as session:
        tasks = [bounded_verify(session, i) for i in range(total)]
        results = await asyncio.gather(*tasks)
    
    successes = [r for r in results if r.success]
    failures = [r for r in results if not r.success]
    
    return BenchmarkResult(
        name="HTTP/1.1 GetVerifyCode",
        total=total,
        success=len(successes),
        failed=len(failures),
        latencies=[r.latency_ms for r in successes]
    )


def load_test_accounts(csv_path: str) -> List[tuple]:
    """加载测试账号"""
    accounts = []
    try:
        with open(csv_path, 'r') as f:
            lines = f.readlines()[1:]  # 跳过表头
            for line in lines:
                parts = line.strip().split(',')
                if len(parts) >= 2:
                    accounts.append((parts[0], parts[1]))
    except FileNotFoundError:
        print(f"警告: 账号文件不存在 {csv_path}，使用默认账号")
        accounts = [("loadtest_01@example.com", "ChangeMe123!")]
    return accounts


async def main():
    print("=" * 60)
    print("MemoChat HTTP 协议压测")
    print("=" * 60)
    print()
    
    # 加载测试账号
    accounts_csv = "D:/MemoChat-Qml-Drogon/Memo_ops/artifacts/loadtest/runtime/accounts/accounts.local.csv"
    accounts = load_test_accounts(accounts_csv)
    
    if not accounts:
        print("错误: 没有可用的测试账号")
        return
    
    print(f"已加载 {len(accounts)} 个测试账号")
    print()
    
    results = []
    
    # 测试1: HTTP 验证码获取 (200请求, 40并发)
    print("[测试 1] HTTP/1.1 获取验证码 (200请求, 40并发)")
    result = await benchmark_http_auth(total=200, concurrency=40, accounts=accounts)
    results.append(result)
    print(f"  成功率: {result.success_rate:.2f}%")
    print(f"  延迟: avg={result.avg_latency:.2f}ms, p50={result.percentile(50):.2f}ms, p90={result.percentile(90):.2f}ms")
    print()
    
    # 测试2: HTTP 登录 (500请求, 100并发)
    print("[测试 2] HTTP/1.1 登录 (500请求, 100并发)")
    result = await benchmark_http_login(total=500, concurrency=100, accounts=accounts)
    results.append(result)
    print(f"  成功率: {result.success_rate:.2f}%")
    print(f"  延迟: avg={result.avg_latency:.2f}ms, p50={result.percentile(50):.2f}ms, p90={result.percentile(90):.2f}ms")
    print()
    
    # 测试3: HTTP 登录高并发 (1000请求, 200并发)
    print("[测试 3] HTTP/1.1 登录高并发 (1000请求, 200并发)")
    result = await benchmark_http_login(total=1000, concurrency=200, accounts=accounts)
    results.append(result)
    print(f"  成功率: {result.success_rate:.2f}%")
    print(f"  延迟: avg={result.avg_latency:.2f}ms, p50={result.percentile(50):.2f}ms, p90={result.percentile(90):.2f}ms")
    print()
    
    # 输出汇总
    print("=" * 60)
    print("压测结果汇总")
    print("=" * 60)
    for r in results:
        print(f"\n{r.name}:")
        print(f"  总请求: {r.total}, 成功: {r.success}, 失败: {r.failed}")
        print(f"  成功率: {r.success_rate:.2f}%")
        print(f"  延迟 (ms):")
        print(f"    平均: {r.avg_latency:.2f}")
        print(f"    最小: {r.min_latency:.2f}")
        print(f"    最大: {r.max_latency:.2f}")
        print(f"    P50:  {r.percentile(50):.2f}")
        print(f"    P90:  {r.percentile(90):.2f}")
        print(f"    P98:  {r.percentile(98):.2f}")
        print(f"    P99:  {r.percentile(99):.2f}")
    
    # 保存结果到 JSON
    report_path = "D:/MemoChat-Qml-Drogon/Memo_ops/artifacts/loadtest/runtime/reports/http_benchmark.json"
    import os
    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    with open(report_path, 'w') as f:
        json.dump([r.to_dict() for r in results], f, indent=2)
    print(f"\n报告已保存到: {report_path}")


if __name__ == "__main__":
    asyncio.run(main())
