# 工具链、测试与 CI

本页覆盖不属于某个业务模块、但会影响全项目开发流程的支撑目录。

## 构建和依赖

| 文件/目录 | 作用 |
| --- | --- |
| `CMakeLists.txt` | 顶层 CMake 构建入口，按 `BUILD_CLIENT`、`BUILD_SERVER`、`BUILD_OPS`、`BUILD_TESTS` 加入子项目 |
| `CMakePresets.json` | Linux/Windows 构建、测试 preset；Linux 默认使用 `linux-full-gcc16` |
| `cmake/` | 共用 CMake helper、格式化目标、模块支持和包重定向 |
| `vcpkg.json`、`vcpkg-configuration.json` | C++ 依赖和 vcpkg registry/source 配置 |
| `compile_commands.json` | 编译数据库入口，供 clangd 等工具使用 |

## 测试

根目录 `tests/` 是持久测试唯一入口：

- `tests/cpp/`：C++ GTest，镜像主项目源码路径。
- `tests/python/`：Python 结构、契约和脚本测试，镜像主项目路径。
- `tests/fixtures/`：跨测试共享数据。

新增测试不要放到业务模块内部的新 `tests/` 目录；历史服务内测试只作为上下文读取。

## 工具脚本

`tools/` 汇集开发和运维工具：

- `tools/scripts/status/`：服务部署、启动、停止、runtime smoke 和客户端启动脚本。
- `tools/loadtest/`：k6/Python/C++ 压测工具。
- `tools/mcps/`：项目自带用户级 MCP server。

脚本改动要保持 Docker-only 依赖和稳定端口，不要把本地凭据写死到源码。

## CI/CD

`.github/` 存放 GitHub Actions 工作流和工作流调用脚本：

- `.github/workflows/`：CI 构建、测试和部署工作流。
- `.github/scripts/`：工作流辅助脚本。

CI 文档只描述入口和职责；具体 gate 以 workflow 文件为准。

## 项目规则

`skills/` 和 `AGENTS.md` 是 AI agent 工作流规则，不是业务代码。改动 skill 时要同步 `skills/_TREE.md`、入口说明和相关引用，避免后续 agent 读取死链或过期规则。
