# 仓库根 目录树

> MemoChat 是一套 QML 客户端 + Drogon 微服务后端的 IM/AI 应用，集成聊天、朋友圈、通话、AI 助手与桌宠等能力。仓库顶层按 `apps/`（客户端与服务端应用）、`infra/`（部署与基础设施）、`tools/`（脚本与工具）、`tests/`（测试）以及 `cmake/`、`docs/`、`skills/` 等支撑目录组织。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`apps/`](apps/_TREE.md) | 客户端（QML）与服务端微服务应用源码 |
| [`infra/`](infra/_TREE.md) | 部署/编排与基础设施配置（k8s、Envoy、可观测性等） |
| [`tools/`](tools/_TREE.md) | 负载测试、MCP server 与运维/开发脚本 |
| [`tests/`](tests/_TREE.md) | 跨服务的测试代码与结构守卫测试 |
| [`cmake/`](cmake/_TREE.md) | 共用 CMake 模块与第三方包重定向 |
| [`docs/`](docs/_TREE.md) | 架构概览、简历与设计/计划文档 |
| [`skills/`](skills/_TREE.md) | 开发/运维工作流的 skill 知识库 |
| [`.github/`](.github/_TREE.md) | GitHub Actions CI/CD 工作流与部署脚本 |

## 构建/缓存目录（未文档化）

`build*/`、`artifacts/`、`.venv/`、`.ai/`、`.git/` 等均为构建、运行或工具生成的产物/缓存，属于生成期内容，不纳入本目录树文档范围。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AGENTS.md` | 面向 AI agent 的项目约定与工作指南 |
| `CMakeLists.txt` | 顶层 CMake 构建入口 |
| `CMakePresets.json` | CMake 预设（含 linux-full-gcc16 等构建配置） |
| `README.md` | 项目说明文档 |
| `pyproject.toml` | Python 工具链/项目配置 |
| `pytest.ini` | pytest 测试配置 |
| `requirements-dev.txt` | 开发期 Python 依赖清单 |
| `vcpkg.json` | vcpkg 依赖清单 |
| `vcpkg-configuration.json` | vcpkg registry/源配置 |
| `compile_commands.json` | 编译数据库（指向构建目录的符号链接） |
| `.clang-format` | C++ 代码格式化规则 |
| `.clangd` | clangd 语言服务器配置 |
| `.dockerignore` | Docker 构建上下文忽略规则 |
| `.editorconfig` | 编辑器统一缩进/编码规则 |
| `.env.example` | 环境变量示例模板 |
| `.gitattributes` | Git 属性（换行/LFS 等）规则 |
| `.gitignore` | Git 忽略规则 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
