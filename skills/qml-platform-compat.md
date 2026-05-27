---
name: memochat-qml-platform-compat
description: Use when a MemoChat QML change must preserve or isolate platform-specific behavior across Linux/WSLg, Windows, macOS, or other graphics backends.
---

# MemoChat QML 平台兼容

用于 QML 或客户端 UI 变更涉及平台差异、兼容路径、专用 wrapper、或只能在特定平台上修复的视觉行为时。

## 规则

- 保留已经正常工作的现有平台路径，先增量添加，再考虑替换。
- 平台修复优先使用 `qml/linux`、`qml/windows`、`qml/macos`、`Qt.platform.os` 检查，或 `#ifdef Q_OS_*` 胶水代码。
- 如果必须触及共享 QML/C++，保持 public API 和默认视觉不变，再用 opt-in 属性或平台专用 wrapper 加新行为。
- 在 `qml.qrc` 中显式注册新平台资源；不要删除或替换旧的平台文件，除非任务明确要求。
- Windows Acrylic/DWM、macOS 路径和其他已知正常平台默认保持原样，除非已经验证需要改动。

## WSLg/Linux 专项

- 不要用未遮罩的 `ShaderEffectSource` 或 `MultiEffect` 去做圆角 glass 或 logo 背景。
- 不要依赖整窗 `layer.*`、`QRegion` mask，或 `Rectangle.clip` 来模拟圆角裁剪。
- 透明无边框窗口优先使用 alpha buffer + QML/Shape 圆角外壳，且让后方内容内缩到曲线内部。
- 如果系统 backdrop blur 不可用，用圆角、抗锯齿的 tint/highlight/border layer 模拟，不要退回方形 backing plate。

## 验证

- 记录被修改的平台边界。
- 至少验证被修改平台，以及任何触及共享路径的平台。
- 视觉问题优先留截图、窗口尺寸、DPI/缩放和实际结果记录。
