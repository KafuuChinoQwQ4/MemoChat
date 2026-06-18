---
name: memochat-task-think
description: Use when a non-trivial MemoChat task needs project-specific skill routing, MemoChat constraints, or persistent .ai task artifacts.
---

# MemoChat Skill 入口

本文件只做入口导航和不可变项目约束。具体流程交给聚焦 skill，避免一次任务加载整套规则。

## 读取顺序

1. 始终先读 `skills/rule.md`。
2. 根据任务只选必要流程 skill：
   - `skills/task.md`：普通功能、修复、重构或项目文档改动。
   - `skills/clarify-first.md`：缺关键决策，不能安全计划或实现。
   - `skills/debugging.md`：bug、测试失败、构建失败、异常行为或连续修复无效。
   - `skills/withtest.md`：需要 CI 可保留测试或运行时测试闭环。
   - `skills/runtime-smoke.md`：部署、启动、服务健康、登录/注册或 full-flow smoke。
   - `skills/review.md`：接收 review 反馈，或完成前复审实际 diff。
   - `skills/release.md`：release、release candidate、commit 或 tag 准备。
   - `skills/planner.md`：创建可复用 `.ai/<name>/prompt.md` 和 `tasks.json`。
   - `skills/skill-authoring.md`：创建、更新、裁剪或复审项目 skill。
   - `skills/tree-doc.md`：新增/删除/移动/重命名文件或文件夹，或文件职责变化时，同步各文件夹的 `_TREE.md` 目录树文档。
3. 根据触及领域只读相关领域 skill：
   - Docker/端口/MCP：`skills/docker-diagnose.md`
   - 数据/迁移/缓存/对象存储/图/向量：`skills/db-migration.md`
   - AI/RAG/工具契约：`skills/ai-rag.md`
   - QML/UI/资源：`skills/qml-ui.md`
   - QML 平台兼容：`skills/qml-platform-compat.md`
   - 图标/SVG：`skills/icon.md`
   - 可观测性：`skills/observability.md`
   - 用户纠正复盘：`skills/reflect.md`
4. 外部快照只在需要时读取入口：
   - `skills/superpowers/SKILL.md`：通用流程，如 brainstorming、TDD、debugging、review、verification。
   - `skills/mattpocock/SKILL.md`：Superpowers 缺口，如架构防腐化、领域语言 grilling、zoom-out、prototype、PRD/issue 切片。
5. 只有构造委派式或基于产物的阶段提示词时，才读取 `skills/PROMPTS.md`。

## 常用外部触发捷径

- 新功能、改行为、需求还需要设计探索：读 `skills/superpowers/brainstorming/SKILL.md`；若重点是领域语言/ADR，改读 `skills/mattpocock/engineering/grill-with-docs/SKILL.md`。
- 看不懂某块代码、要调用链/模块地图/上层视角：读 `skills/mattpocock/engineering/zoom-out/SKILL.md`。
- 代码防腐化、架构退化、耦合收敛、深模块、可测试性：读 `skills/mattpocock/engineering/improve-codebase-architecture/SKILL.md`。
- 原型、状态机试跑、UI 多方向尝试：读 `skills/mattpocock/engineering/prototype/SKILL.md`。
- PRD、issue、任务切片、triage：读 `skills/mattpocock/engineering/to-prd/SKILL.md`、`to-issues/SKILL.md` 或 `triage/SKILL.md`。
- TDD、红绿重构、先写测试：读 `skills/withtest.md` 和 `skills/superpowers/test-driven-development/SKILL.md`。
- bug、测试失败、构建失败、连续修复无效：读 `skills/debugging.md` 和 `skills/superpowers/systematic-debugging/SKILL.md`。
- 完成前声明修复/通过/完成：读 `skills/superpowers/verification-before-completion/SKILL.md`。
- review 反馈或完成前代码审查：读 `skills/review.md`，再按场景读 Superpowers review 子 skill。
- 高风险改动需要隔离：读 `skills/superpowers/using-git-worktrees/SKILL.md`。
- 交接给后续代理：读 `skills/mattpocock/productivity/handoff/SKILL.md`。
- 用户要求极简、少 token 或 caveman mode：读 `skills/mattpocock/productivity/caveman/SKILL.md`。

## 不可变项目约束

- 默认工作目录是 `/root/code/MemoChat-Qml-Drogon-linux`，默认 Linux 运行时是 WSL 发行版 `archlinux`。
- 从 Windows 调 Linux 命令时使用：

```powershell
wsl -d archlinux -- bash -lc 'cd /root/code/MemoChat-Qml-Drogon-linux && source /root/.memochat-linux-env && <command>'
```

- 不要使用 `wsl -d Arch`。Windows/PowerShell 只用于旧版 Windows 客户端检查、Docker Desktop 迁移/备份，或用户明确要求的 Windows 侧工作。
- 基础设施依赖必须在 Docker 中。不要在 Docker 外安装或启动 Redis、Postgres、MongoDB、RabbitMQ、Redpanda、MinIO、Neo4j、Qdrant、观测或 AI/RAG 依赖。
- Arch 原生 Docker 是默认运行时；加载 `/root/.memochat-linux-env`，使用 `/var/run/docker.sock`。
- 保持 Docker 发布端口稳定；除非任务明确要求，否则不要改 compose 端口映射。
- Linux 缓存、模型、vcpkg、Qt 产物和大文件优先放 `/data`；Docker 绑定数据使用 `/data/docker-data/memochat`。
- 新增或迁移持久化测试只放仓库根 `tests/`：C++ 放 `tests/cpp/<主项目相对路径>/...`，Python 放 `tests/python/<主项目相对路径>/...`，fixtures 放 `tests/fixtures/...`。
- 绝不 reset、checkout、clean 或 revert 用户改动；脏工作区中只碰任务相关文件。

## 默认构建入口

触及 Linux 部署、运行时或完整验证时使用：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
ctest --preset linux-full-gcc16 --output-on-failure
```

`tools/scripts/status/deploy_services.sh` 默认从 `build-linux-full-gcc16/bin` 复制服务和客户端产物。

## 任务产物

非平凡任务使用 `.ai/<project>/<letter>/` 记录上下文、计划、验证和复审。`skills/task.md` 拥有具体布局；并行工作线规则由 `skills/parallel-agents.md` 拥有；运行时测试循环由 `skills/withtest.md` 拥有。
