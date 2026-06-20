---
name: memochat-no-backward-compat
description: Use when removing backward-compatibility code (old client/data/protocol support — dual-reads, field aliases, version-branched logic, migration reads, deprecated endpoints), or deciding whether a fallback you're about to add is one. NOT for platform/library adapters, error-tolerance defaults, or the forced-update gate. MemoChat 3.0 不兼容 2.0。
---

# MemoChat 不向后兼容

项目规定（`skills/rule.md`）：不为旧版本/旧客户端/旧数据格式保留兼容路径，旧客户端靠强制更新拦截。本 skill 给删除时的分类与验证流程，核心是**别误删本不该删的**。

## 先分类：「兼容」有五类互斥含义，只删 E

判断一段代码属哪类，看它**为谁存在**，不看它叫什么（"compat""legacy""fallback"会出现在任意一类里）：

- **A 强制更新机制（保留）**：按客户端版本号拦截/放行的闸门（如 `IsClientVersionAllowed`、`min_version`、`ClientVersionTooLow`、`kMinClientVersion`）。这是"强制用户更新"功能本身，不是要删的兼容。
- **B 平台/库适配（保留）**：跨 OS 或第三方库差异的封装（`*Compat.h`、平台专用 QML、`Qt.platform.os`、`#ifdef Q_OS_*`）。与版本无关。
- **C 运行时错误兜底（保留）**：值缺失/后端不可用时的默认与降级（`value_or`、默认值、"backend unavailable → fallback"、一个通用结构对多种**当前**数据形态的归一）。是容错，不是版本兼容。
- **D 旧传输架构（默认不动）**：opt-in、被契约测试锁定的退役传输层（见 `apps/server/core/docs/gate-compatibility-inventory.md`）。改它要同步那份清单。
- **E 旧客户端/旧数据兼容（要删的）**：只为让**更老的客户端/数据/协议**继续工作而存在的代码——双读（读新回退旧）、旧字段别名、`schemaVersion`/格式版本分支、历史数据迁移读、已废弃端点。

**C 与 E 的边界最难、最易判错**（上一轮清理中工作流在此反复判反方向）。同一个 `a || b` 既可能是"新字段回退旧字段"(E)，也可能是"一个模型适配多种当前形态"(C)。拿不准时**默认当作 C 保留**，走下面的双向验证再定——误删 C 会引入运行时 bug，漏删一个 E 只是稍欠干净。

## 删除 E 的流程（按序，缺一步就可能改错）

1. **定方向**：查当前发送/写入端**实际用哪个 key/格式**，以此确定谁是规范、谁是旧。别假设"回退链第一个是新的"——方向常与直觉相反。
2. **对齐契约两端**：按 command id / 端点 / 数据消费点，把"生产方 ↔ 消费方"对上。同名字段在不同契约里可能语义不同，分别处理，不按名字一刀切。
3. **全量枚举**：同一模式常散布多个文件（cpp/QML/JS/py/测试/配置），自己再全量 grep，一个 helper 的所有调用点都要覆盖。
4. **两端统一 + 同步测试**：生产方与消费方都切到新形态，更新被锁定的契约测试。
5. **不确定就停**：若两端都在用、生产方自己都没统一（字段未规范化），那是独立的规范化重构，不在本清理范围；遥测/日志取值不是协议字段，不动。

## 不该触发本 skill 的场景

修平台显示差异 → `qml-platform-compat.md`；加错误兜底/默认值（C）→ 普通实现；调强制更新版本阈值（A）→ 那是功能不是兼容；动 legacy transport 目录（D）→ 先看 inventory 清单。

## 验证闭环

- 全量构建 + ctest：见 `skills/memochat-build-verify-workflow`（`cmake --preset linux-full-gcc16` → build → `ctest --preset linux-full-gcc16`）。
- 若契约测试本就有 pre-existing 失败：用 `git stash` 跑 baseline 对比，确认失败集改动前后一致（零新增回归），而非看绝对失败数。
