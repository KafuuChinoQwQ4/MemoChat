# Windows Service Scripts

This folder contains Windows batch entrypoints for local MemoChat service orchestration.

- `start_test_services.bat`
- `stop_test_services.bat`

Examples:

```bat
scripts\windows\start_test_services.bat --no-client
scripts\windows\start_test_services.bat --no-client --skip-ops
scripts\windows\stop_test_services.bat
```

Flags:

- `--no-client`: do not start the QML client
- `--skip-ops`: do not start the Memo_ops platform
- `--foreground`: reserved for future interactive mode; default launch path is background-oriented
