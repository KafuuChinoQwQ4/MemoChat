Verification:

- `cmake --preset msvc2022-client-verify`: passed.
- `cmake --build --preset msvc2022-client-verify`: passed.
- `git diff --check`: passed with existing LF/CRLF warnings only.

No Docker/MCP checks were required because this is a client-only rendering change with no infrastructure or persisted data changes.
