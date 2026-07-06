# 开发入口

MemoChat 当前默认面向 WSL/Arch Linux 开发，基础设施依赖必须在 Docker 中运行。日常开发优先使用 `linux-full-gcc16` preset 和 `tools/scripts/status/` 下的脚本。

## 常用路径

| 路径 | 作用 |
| --- | --- |
| `apps/server/core/` | C++ 服务端与 Python AIOrchestrator |
| `apps/client/desktop/MemoChat-qml/` | Qt/QML 桌面客户端 |
| `apps/web/` | React/Vite Web 前端 |
| `infra/deploy/local/` | 本地 Docker 依赖、Envoy 和观测栈 |
| `tools/scripts/status/` | 构建产物部署、服务启动停止、客户端启动脚本 |
| `tests/` | 跨模块 C++/Python 测试 |

## 推荐顺序

1. 先读 [environment.md](environment.md)，确认工具链和 Docker 依赖。
2. 再读 [compile-test-run.md](compile-test-run.md)，掌握构建和测试命令。
3. 需要完整本地运行时，读 [local-runtime.md](local-runtime.md)。
4. 修改代码前读 [conventions.md](conventions.md)，避免破坏项目边界。
