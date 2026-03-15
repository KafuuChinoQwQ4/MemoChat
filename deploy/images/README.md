# Image Build Files

This folder contains container build inputs only.

- `services/cpp-service.Dockerfile`: shared image build for `GateServer`, `StatusServer`, and `ChatServer`
- `services/varifyserver.Dockerfile`: Node image for `VarifyServer`
- `services/memo-ops.Dockerfile`: Python image for `Memo_ops`
- `common/entrypoints/server-entrypoint.sh`: runtime entrypoint for C++ services
