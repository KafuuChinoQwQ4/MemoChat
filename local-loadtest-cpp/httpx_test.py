import asyncio, httpx, json, time

async def test():
    client = httpx.AsyncClient(timeout=httpx.Timeout(5.0), http2=False, limits=httpx.Limits(max_connections=200, max_keepalive_connections=100))
    accounts = [('loadtest_01@example.com', 'ChangeMe123!')]
    for email, password in accounts:
        body = {'email': email, 'passwd': password, 'client_ver': '2.0.0'}
        body_bytes = json.dumps(body).encode()
        t0 = time.perf_counter()
        try:
            resp = await client.post('http://127.0.0.1:8080/user_login', content=body_bytes, headers={'Content-Type': 'application/json'})
            data = resp.content
            r = json.loads(data)
            print('error=%s uid=%s time=%.1fms' % (r.get('error'), r.get('uid'), (time.perf_counter()-t0)*1000))
            print('body=%s' % data.decode()[:300])
        except Exception as e:
            print('exception: %s: %s' % (type(e).__name__, e))
    await client.aclose()

asyncio.run(test())
