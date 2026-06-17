const { spawnSync } = require('child_process');
const path = require('path');

const script = path.join(__dirname, 'test_register_login.ps1');
const args = [
    '-NoProfile',
    '-ExecutionPolicy',
    'Bypass',
    '-File',
    script
];

if (process.argv[2]) {
    args.push('-Email', process.argv[2]);
}

const result = spawnSync('powershell', args, { stdio: 'inherit' });
process.exit(result.status ?? 1);
