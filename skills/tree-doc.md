---
name: memochat-tree-doc
description: Use when adding, deleting, moving, or renaming any file or folder, or when a file's responsibility changes, so the per-folder _TREE.md directory docs stay in sync.
---

# MemoChat 目录树文档维护

仓库每个源码文件夹根部都有一个 `_TREE.md`，列出该层**直接子项**（直接子目录 + 直接文件）并逐项一句话概括作用。它们靠人/代理实时维护，不是自动生成。任何改动文件树或文件职责的任务，收尾时都必须同步对应的 `_TREE.md`。

## 何时触发

满足任一条就要更新 `_TREE.md`：

- 新增文件或文件夹。
- 删除文件或文件夹。
- 移动或重命名文件/文件夹。
- 某个文件/文件夹的职责发生明显变化，原概括已不准确。

## 必须改哪些 `_TREE.md`

- **新增子项 X 到目录 D**：在 `D/_TREE.md` 对应表（子目录表 / 文件表）加一行；如果 X 是文件夹，还要在 `X/_TREE.md` 新建该文件夹的目录树文档。
- **删除目录 D 下的子项 X**：从 `D/_TREE.md` 删除那一行；如果 X 是文件夹，它的 `X/_TREE.md` 随文件夹一起删除（无需单独保留）。
- **移动/重命名**：等于在旧父目录删一行、在新父目录加一行；文件夹被移动时其内部 `_TREE.md` 跟随移动，链接相对路径保持正确。
- **职责变化**：更新该子项所在父目录 `_TREE.md` 的概括文字；如果改的是文件夹整体定位，还要更新该文件夹自己 `_TREE.md` 顶部的 `>` 一句话概括。

只更新直接父目录那一层；不要因为一个深层文件改动去改所有祖先文档（每层只描述自己的直接子项）。

## 格式约束（与现有 `_TREE.md` 保持一致）

- 文件名固定为 `_TREE.md`，绝不改写或并入 `README.md`。
- 一级标题：`# <文件夹名>/ 目录树`（仓库根用 `# 仓库根 目录树`）。
- 紧跟一行 `>` 引用块：一句话概括整个文件夹职责。
- `## 子目录` 表：每个直接子文件夹一行，链接 `[`<名>/`](<名>/_TREE.md)`；本层无子目录则省略整段。
- `## 文件` 表：每个直接文件一行，文件名用反引号、**不**加链接；本层无文件则省略整段。
- `_TREE.md` 自身不列入文件表。
- 指向「不在文档范围内」的子目录（外部 skill 快照 `skills/superpowers`、`skills/mattpocock`、`skills/awesomedesign`，运行期 `artifacts/`、二进制资源 `live2d/` 等）只写纯文本行 + 「未文档化」标注，**不要**加 `_TREE.md` 链接（否则成为死链）。
- 末尾保留标记行：`<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->`。
- 概括用中文，简洁说「大概作用」，不逐行复述代码。

## 文档范围（哪些文件夹需要 `_TREE.md`）

- 纳入：被 git 跟踪的源码文件夹，覆盖 `apps/`、`infra/`、`tools/`、`tests/`、`cmake/`、`docs/`、`skills/`（仅本层）、`.github/` 及仓库根。
- 排除：`build*/`、`.venv/`、`.ai/`、`artifacts/` 与运行期产物、各类缓存（`.pytest_cache/`、`.ruff_cache/`、`__pycache__/`）、`.git/`，以及外部 skill 快照 `skills/superpowers/**`、`skills/mattpocock/**`、`skills/awesomedesign/**`。
- 新建的文件夹若落在「纳入」范围，就要补 `_TREE.md`；落在「排除」范围则不需要。

## 收尾验证

改完后自检：

1. 受影响的父目录 `_TREE.md` 表格行数 == 该层直接子项数（文件数不含 `_TREE.md`，含 `README.md` 与跟踪的点文件如 `.dockerignore`）。
2. 新增文件夹下已存在 `_TREE.md`；删除的文件夹其 `_TREE.md` 已随之移除。
3. 所有 `[...](.../_TREE.md)` 链接都能解析到真实文件（无死链）。
4. `git diff --check` 无空白错误。

点文件（`.dockerignore`、`.clang-format`、`.env.example` 等被 git 跟踪的隐藏文件）也要列入文件表；用 `ls -pa` 或 `git ls-files <dir>` 核对，避免 `ls` 漏掉点文件。
