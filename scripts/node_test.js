const http = require('http');

async function request(method, path, body) {
    return new Promise((resolve, reject) => {
        const postData = JSON.stringify(body);
        const options = {
            hostname: '127.0.0.1',
            port: 8080,
            path: path,
            method: method,
            headers: {
                'Content-Type': 'application/json',
                'Content-Length': Buffer.byteLength(postData)
            }
        };
        const req = http.request(options, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => resolve({ status: res.statusCode, body: data }));
        });
        req.on('error', reject);
        req.write(postData);
        req.end();
    });
}

async function main() {
    const email = process.argv[2] || 'testuser123@loadtest.local';
    const Redis = require('D:/MemoChat-Qml-Drogon/server/VarifyServer/node_modules/ioredis');

    // Step 1: Get verify code
    console.log('\n=== Step 1: Get verify code ===');
    const r1 = await request('POST', '/get_varifycode', { email });
    console.log('Status:', r1.status);
    console.log('Body:', r1.body);

    // Step 2: Poll Redis for the code
    console.log('\n=== Step 2: Poll Redis for code ===');
    const cli = new Redis({ host: '127.0.0.1', port: 6379, password: '123456' });
    let code = null;
    for (let i = 0; i < 10; i++) {
        await new Promise(r => setTimeout(r, 200));
        code = await cli.get('code_' + email);
        if (code) {
            console.log('Found code:', code);
            break;
        }
    }
    await cli.quit();

    if (!code) {
        console.log('No code found in Redis!');
        return;
    }

    // Step 3: Register user
    console.log('\n=== Step 3: Register user ===');
    const r2 = await request('POST', '/user_register', {
        email,
        user: 'testuser',
        passwd: '123456',
        confirm: '123456',
        icon: '',
        varifycode: code,
        sex: 0
    });
    console.log('Status:', r2.status);
    console.log('Body:', r2.body);

    // Step 4: Login
    console.log('\n=== Step 4: Login ===');
    const r3 = await request('POST', '/user_login', {
        email,
        passwd: '123456',
        client_ver: '3.0.0'
    });
    console.log('Status:', r3.status);
    console.log('Body:', r3.body);
}

main().catch(e => { console.error(e); process.exit(1); });
