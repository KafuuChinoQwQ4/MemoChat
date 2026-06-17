const { spawnSync } = require('child_process');
const path = require('path');

const email = process.argv[2] || 'testuser123@loadtest.local';
const dockerCli = path.join(__dirname, '..', 'docker', 'arch-docker.cmd');
const result = spawnSync(
    dockerCli,
    ['exec', '-e', 'REDISCLI_AUTH=123456', 'memochat-redis', 'redis-cli', 'GET', `code_${email}`],
    { encoding: 'utf8' }
);

if (result.status !== 0) {
    process.stderr.write(result.stderr || 'redis-cli failed\n');
    process.exit(result.status ?? 1);
}

const code = result.stdout.trim();
console.log('Verify code:', code === '(nil)' ? '' : code);
