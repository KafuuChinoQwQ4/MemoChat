---
name: memochat-release
description: Use when preparing a MemoChat release or release candidate, validating the worktree, running selected build/test checks, or creating a release commit or tag.
---

# MemoChat 发布

用于为本仓库准备 release 或 release candidate。

## 前置条件

1. 运行 `git status --short`。
2. 如果存在无关未提交改动，先确认这些改动是否属于本次发布；只有在会影响发布内容、验证或提交范围时才停止并询问如何处理。
3. 确认发布范围：
   - server
   - client
   - ops
   - full stack
4. 确认用户想要 commit、tag、二者都要，还是只要 release note。

## 收集变更

使用：

```bash
git log --oneline --decorate -n 50
git diff --stat
```

如果存在上一个 tag，从该 tag 起比较：

```bash
git tag --sort=-v:refname
git log <tag>..HEAD --oneline
```

编写面向用户的要点。跳过嘈杂的中间工作、生成缓存、本地 `.ai/` 产物和 Docker 数据。

## 验证 Docker 运行时假设

检查固定端口和容器健康：

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
```

不要把修改 compose 端口作为发布清理的一部分。

## 构建/测试矩阵

`archlinux` WSL 发布/运行时部署前使用 Linux full 构建。部署脚本从 `build-linux-full-gcc16/bin` 复制。

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

发布范围需要自动化测试时，运行 test preset：

```bash
ctest --preset linux-full-gcc16 --output-on-failure
```

需要运行时发布信心时，使用现有服务脚本和 smoke 测试：

```bash
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

需要时仍可从 Windows 运行旧版 smoke 探针：

```powershell
tools/scripts/verify/test_register_login.ps1
tools/scripts/verify/test_login.ps1
tools/scripts/verify/full_flow_test.ps1
```

遇到端口冲突或 Docker 依赖失败时停止，并报告占用进程/容器。

## Release Note 形态

使用简洁要点并按以下分组：

- Added
- Changed
- Fixed
- Ops / Infrastructure

除非影响部署或运维，否则不要写实现细节。

## Commit/Tag

只有在用户明确批准最终 release 文本后，才 commit 或 tag。

发布门槛比普通实现任务更严格。它只适用于 release / release candidate 流程，不覆盖日常功能开发、调试或文档工作。

提交前：

- 确保 `.ai/` 产物没有被 staged，除非明确要求
- 确保 ignored Docker 数据和本地缓存没有被 staged
- 在最终消息中包含验证命令和结果
