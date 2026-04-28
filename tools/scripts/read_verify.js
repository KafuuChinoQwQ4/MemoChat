// Get verify code from Redis
const Redis = require('D:/MemoChat-Qml-Drogon/server/VarifyServer/node_modules/ioredis');

const cli = new Redis({
    host: '127.0.0.1',
    port: 6379,
    password: '123456',
});

async function main() {
    const email = process.argv[2] || 'testuser123@loadtest.local';
    const key = 'code_' + email;
    console.log('Looking up key:', key);
    const code = await cli.get(key);
    console.log('Verify code:', code);
    await cli.quit();
}
main().catch(e => { console.error(e); process.exit(1); });
