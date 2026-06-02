const { spawnSync } = require('child_process');
const path = require('path');

const script = path.join(__dirname, 'test_register_login.ps1');
const result = spawnSync(
    'powershell',
    ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', script],
    { stdio: 'inherit' }
);

process.exit(result.status ?? 1);
