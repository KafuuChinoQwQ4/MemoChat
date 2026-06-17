const { spawnSync } = require('child_process');
const path = require('path');

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function readCode(email) {
    const dockerCli = path.join(__dirname, '..', 'docker', 'arch-docker.cmd');
    const result = spawnSync(
        dockerCli,
        ['exec', '-e', 'REDISCLI_AUTH=123456', 'memochat-redis', 'redis-cli', 'GET', `code_${email}`],
        { encoding: 'utf8' }
    );
    if (result.status !== 0) {
        return null;
    }
    const code = result.stdout.trim();
    return code && code !== '(nil)' ? code : null;
}

async function main() {
    const email = process.argv[2] || 'testuser123@loadtest.local';
    for (let i = 0; i < 5; i++) {
        await sleep(500);
        const code = readCode(email);
        if (code) {
            console.log('Verify code found:', code);
            return;
        }
    }
    console.log('No verify code found in Redis for:', email);
}

main().catch(e => { console.error(e); process.exit(1); });
