const Redis = require('D:/MemoChat-Qml-Drogon/server/VarifyServer/node_modules/ioredis');

async function getCode(email) {
    const cli = new Redis({
        host: '127.0.0.1',
        port: 6379,
        password: '123456',
    });
    const key = 'code_' + email;
    const code = await cli.get(key);
    await cli.quit();
    return code;
}

async function main() {
    const email = process.argv[2] || 'testuser123@loadtest.local';
    // Read multiple times to catch the code
    for (let i = 0; i < 5; i++) {
        await new Promise(r => setTimeout(r, 500));
        const code = await getCode(email);
        if (code) {
            console.log('Verify code found:', code);
            return;
        }
    }
    console.log('No verify code found in Redis for:', email);
}
main().catch(e => { console.error(e); process.exit(1); });
