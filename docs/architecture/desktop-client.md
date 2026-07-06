# 桌面客户端模块

桌面客户端位于 `apps/client/desktop/MemoChat-qml/`，目标为 `MemoChatQml`。Qt/QML 客户端默认使用 C++23，Qt kit 以当前 `linux-full-gcc16` preset 的 Qt 6.8.3 路径为准。

## 目录职责

| 目录 | 职责 |
| --- | --- |
| `app/` | 应用壳、启动、composition root、会话、连接、窗口和跨 feature 协调 |
| `core/` | 通用能力：网络、session、媒体、公共工具 |
| `features/` | auth、chat、contact、group、moments、profile、call、agent、pet、settings、r18 等业务模块 |
| `qml/` | QML shell、共享组件、Linux/WSLg 平台 UI 路径 |
| `shared/` | 跨模块 UI、媒体、gateway、工具函数 |
| `resources/` | 图标、QRC、Live2D、Web 资源 |
| `live2d/` | Live2D 模型、渲染和资产处理 |
| `platform/` | 平台专用适配 |
| `docs/` | 客户端边界文档和结构模板 |

## 边界原则

`AppController` 是应用壳和跨模块入口，不是业务中枢。功能内的请求、响应、分页、发送、草稿、缓存和事件处理应留在 `features/<name>/controller|service|model|viewmodel`。详细规则见 `apps/client/desktop/MemoChat-qml/docs/client-architecture-boundaries.md`。

QML 只做视觉和用户意图转发，不直接拼业务 payload，不绕过 feature facade 写跨模块状态。Linux/WSLg 特有窗口和透明圆角问题优先放 `qml/linux` 或平台专用 wrapper。
