const redis = require('redis');
const { promisify } = require('util');

async function main() {
    const client = redis.createClient({
        host: '127.0.0.1',
        port: 6379,
        password: '123456'
    });
    const getAsync = promisify(client.get).bind(client);
    
    const code = await getAsync('code_test@test.com');
    console.log('Verify code for test@test.com:', code);
    
    client.quit();
}
main().catch(console.error);
