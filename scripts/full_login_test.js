const http = require('http');
const path = require('path');
process.env.NODE_PATH = path.join(__dirname, '..', 'server', 'VarifyServer', 'node_modules');
require('module').Module._initPaths();

const Redis = require('ioredis');
const redis = new Redis({ host: '127.0.0.1', port: 6379, password: '123456' });

const email = 'pgtest002@loadtest.local';
const passwd = '123456';
const nick = 'pgtest002';
const client_ver = '3.0.0';

function postJson(url, payload) {
    return new Promise((resolve, reject) => {
        const urlObj = new URL(url);
        const postData = JSON.stringify(payload);
        const options = {
            hostname: urlObj.hostname,
            port: urlObj.port,
            path: urlObj.pathname,
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Content-Length': Buffer.byteLength(postData)
            },
            timeout: 10000
        };
        const req = http.request(options, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(data));
                } catch (e) {
                    resolve({ raw: data });
                }
            });
        });
        req.on('error', reject);
        req.on('timeout', () => { req.destroy(); reject(new Error('timeout')); });
        req.write(postData);
        req.end();
    });
}

async function waitForCode(email, maxWait = 15000) {
    const start = Date.now();
    while (Date.now() - start < maxWait) {
        const code = await redis.get(`code_${email}`);
        if (code) return code;
        await new Promise(r => setTimeout(r, 500));
    }
    return null;
}

async function main() {
    console.log(`[1] Requesting verify code for ${email}...`);
    const regResult = await postJson('http://127.0.0.1:8080/user_register', {
        email, nick, passwd, client_ver
    });
    console.log('[1] Register result:', JSON.stringify(regResult, null, 2));

    if (regResult.error !== 0 && regResult.error !== undefined) {
        console.log('[!] Registration returned error:', regResult.error);
        // Try login instead
        console.log('[2] Trying login...');
        const loginResult = await postJson('http://127.0.0.1:8080/user_login', {
            email, passwd, client_ver
        });
        console.log('[2] Login result:', JSON.stringify(loginResult, null, 2));
        redis.disconnect();
        return;
    }

    console.log('[2] Waiting for verify code from Redis...');
    const code = await waitForCode(email);
    if (!code) {
        console.log('[X] No code found in Redis!');
        redis.disconnect();
        return;
    }
    console.log(`[2] Got code: ${code}`);

    const verifyResult = await postJson('http://127.0.0.1:8080/get_varifycode', {
        email, code, client_ver
    });
    console.log('[3] Verify result:', JSON.stringify(verifyResult, null, 2));

    console.log('[4] Logging in...');
    const loginResult = await postJson('http://127.0.0.1:8080/user_login', {
        email, passwd, client_ver
    });
    console.log('[4] Login result:', JSON.stringify(loginResult, null, 2));

    redis.disconnect();
}

main().catch(e => {
    console.error('Error:', e.message);
    redis.disconnect().catch(() => {});
});
