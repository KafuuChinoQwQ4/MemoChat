---
name: memochat-qml-ui
description: Use when implementing or reviewing MemoChat QML UI, MemoOps UI, shared client components, resources, icons, layouts, visuals, or client build checks.
---

# MemoChat QML UI

用于 MemoChat 桌面 QML、MemoOps QML、共享客户端组件、图标、资源、布局修复和视觉行为。

## 相关区域

- `apps/client/desktop/MemoChat-qml`
- `apps/client/desktop/MemoChat-qml/shared`
- `apps/client/desktop/CMakeLists.txt`
- `infra/Memo_ops/client/MemoOps-qml`
- `infra/Memo_ops/config/memoops-qml.ini`
- `apps/server/config/memoops-qml.ini`

## 发现

添加新 UI 前搜索现有模式：

```bash
rg -n "<ComponentName>|property |signal |Loader|StackView|ListView|Repeater|Image|Button" apps/client infra/Memo_ops/client
rg -n "qml.qrc|resources|icons|assets" apps/client infra/Memo_ops/client
```

检查 UI 是否依赖服务端端点或运行时配置。如果依赖，将这些内容纳入计划。

## 设计规则

- 匹配现有 QML 组件模式和间距。
- 在窄窗口中保持文本适配控件。
- 使用现有图标/资源约定。
- 除非项目已有全局样式系统，否则不要新增。
- 对有状态控件，适用时处理 normal、hover、pressed、disabled、loading 和 error 状态。
- 配置已提供服务 URL 时，不要硬编码服务 URL。
- 跨平台 UI 兼容规则统一见 `skills/rule.md`；平台专用修复再读 `skills/qml-platform-compat.md`。

## 实现

- 同步更新 QML 和资源注册。
- 保持 C++/QML 接口属性名一致。
- 如果添加新的后端数据，同时更新 DTO/API/服务端响应处理。
- 临时截图或记录放到 `.ai/<project>/<letter>/screenshots` 或 logs 下。

## 验证

触及 QML/C++ 客户端产物、资源注册或客户端/服务端契约时，优先构建 full preset，确保 QML、服务端契约和运行时产物来自同一构建树。纯 SVG/文案/静态资源检查可先用最窄资源检查、`qmllint` 或读回验证，并记录为什么不需要 full preset。

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

运行时 UI 检查中，如果屏幕需要后端数据，先启动必需的 Linux 服务。可行时使用截图。如果没有自动化 UI runner，记录准确的手工视觉检查。明确的 Windows 客户端检查使用旧版 `msvc2022-full` 和 `.bat/.ps1` 流程。

视觉或布局改动不能只用“构建通过”作为完成证据。验证日志应包含至少一种适用证据：

- 截图路径，优先放在 `.ai/<project>/<letter>/screenshots/`。
- `qmllint`、资源检查或客户端启动命令及输出。
- 手工检查记录：平台、窗口尺寸、DPI/缩放、数据状态、预期视觉结果和实际结果。

跨平台兼容修复要分别记录被修改平台和任何触及共享路径的平台；无法验证的平台必须列为剩余风险。

## 复审清单

- 预期窗口尺寸下没有文本重叠
- 没有缺失资源路径
- 没有破坏的 import
- 没有硬编码本地专用路径，除非它是已记录的开发配置
- 客户端/服务端契约保持一致
- 相关位置处理 error/loading/empty 状态
- 视觉证据已写入验证日志，或明确记录无法截图/启动的原因
