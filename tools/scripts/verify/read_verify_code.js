const redis = require('redis');
const { promisify } = require('util');

async function main() {
    const password = process.env.MEMOCHAT_REDIS_PASSWORD;
    if (!password) {
        throw new Error('Missing MEMOCHAT_REDIS_PASSWORD');
    }
    const client = redis.createClient({
        host: process.env.MEMOCHAT_REDIS_HOST || '127.0.0.1',
        port: Number(process.env.MEMOCHAT_REDIS_PORT || 6379),
        password
    });
    const getAsync = promisify(client.get).bind(client);
    
    const code = await getAsync('code_test@test.com');
    console.log('Verify code for test@test.com:', code);
    
    client.quit();
}
main().catch(console.error);
