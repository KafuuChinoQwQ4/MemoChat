---
description: 实现或复审 MemoChat QML UI 变更、资源、图标、客户端构建验证以及视觉/运行时检查。
---

# MemoChat QML UI

用于 MemoChat 桌面 QML、MemoOps QML、共享客户端组件、图标、资源、布局修复和视觉行为。

## 相关区域

- `apps/client/desktop/MemoChat-qml`
- `apps/client/desktop/MemoChatShared`
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
- 跨平台 UI 变更优先兼容性。修复 Linux/WSLg、macOS、Windows、Wayland/X11、DPI、字体、compositor 或 graphics-backend 差异时，保留已经正常工作的现有平台行为，并通过平台专用 QML 文件夹、平台保护的 C++/QML 分支、资源 alias 或窄兼容组件添加修复。除非证明 bug 在这些平台共享路径上，否则不要重写共享组件或替换正常工作的视觉路径。
- 如果平台修复必须触及共享 QML 组件，保持其 public API 和默认渲染与当前正常工作平台兼容，然后为新行为添加 opt-in properties 或平台专用 wrapper。在计划/复审中记录平台边界，并至少验证被修改平台，以及任何共享路径被触及的平台。
- 优先使用增量平台结构：`qml/linux`、`qml/windows`、`qml/macos`、`Qt.platform.os` 检查，或 `#ifdef Q_OS_*` C++ glue。在 `qml.qrc` 中明确注册平台资源，添加新兼容路径时避免删除或替换旧平台文件。
- 对平台 glass/acrylic 修复，Windows/macOS 保持在其现有平台路径上，将 WSLg/Linux 兼容性放在 `qml/linux` 或平台保护代码中。不要用 Linux fallback 替换 Windows DWM Acrylic。
- 在 WSLg/Linux 上，不要使用未遮罩的 `ShaderEffectSource`/`MultiEffect` 捕获，因为它们会在圆角 glass 控件或 logo 后产生矩形 backing plate。如果系统 backdrop blur 不可用，用圆角、抗锯齿的 tint/highlight/border layer 模拟 glass。
- 透明无边框 Linux 窗口必须保留圆角抗锯齿：每个可见 backdrop/control layer 需要匹配 radius、`antialiasing: true`，并且后面没有方形 parent fill。优先使用 alpha buffer 加 QML 绘制的圆角 layer，并留小的透明 inset；避免整窗 QML `layer.*`，也避免为 soft glass 边缘使用 `QRegion` mask，因为二者在 WSLg 下都可能产生可见矩形或锯齿边缘。
- 对 WSLg 顶层圆角窗口，不要依赖 `Rectangle.clip` 将子页面裁剪成圆角；它会裁剪到矩形 item bounds。使用仅 Linux 的 `QtQuick.Shapes` 圆角 shell 或等价抗锯齿路径，然后把方形页面/backdrop 内容内缩到足够进入曲线内部的位置。

## 实现

- 同步更新 QML 和资源注册。
- 保持 C++/QML 接口属性名一致。
- 如果添加新的后端数据，同时更新 DTO/API/服务端响应处理。
- 临时截图或记录放到 `.ai/<project>/<letter>/screenshots` 或 logs 下。

## 验证

Linux 客户端检查也统一构建 full preset，确保 QML、服务端契约和运行时产物来自同一构建树。

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

运行时 UI 检查中，如果屏幕需要后端数据，先启动必需的 Linux 服务。可行时使用截图。如果没有自动化 UI runner，记录准确的手工视觉检查。明确的 Windows 客户端检查使用旧版 `msvc2022-full` 和 `.bat/.ps1` 流程。

## 复审清单

- 预期窗口尺寸下没有文本重叠
- 圆角 glass 控件、logo 或透明窗口角后没有方形 backing plate
- 平台专用兼容修复是增量式的，并且不会改变 Windows Acrylic 等正常工作的现有平台路径，除非这是明确意图且已验证
- 为平台修复触及的共享 QML/C++ 保留现有默认行为，并暴露 opt-in 兼容行为
- 没有缺失资源路径
- 没有破坏的 import
- 没有硬编码本地专用路径，除非它是已记录的开发配置
- 客户端/服务端契约保持兼容
- 相关位置处理 error/loading/empty 状态
