---
name: memochat-qml-platform-compat
description: Use when a MemoChat QML change must preserve or isolate platform-specific behavior across Linux/WSLg, Windows, macOS, or other graphics backends.
---

# MemoChat QML 平台兼容

平台兼容的**核心规则**(compat-first、保留可用平台路径、共享代码用 opt-in 属性/平台 wrapper、`qml/<platform>`+`Qt.platform.os`+`#ifdef Q_OS_*` 结构、`qml.qrc` 显式注册、WSLg 透明圆角的 ShaderEffectSource/MultiEffect/`layer.*`/QRegion/`Rectangle.clip` 禁用项)统一见 `skills/rule.md` § UI 编写规则,不在此重复。

本文件只补充该规则之外的**验证要求**:

- 记录本次修改触及的平台边界。
- 验证被修改的平台,以及任何被触碰的共享路径所影响的其它平台。
- 视觉问题优先留证据:截图、窗口尺寸、DPI/缩放、实际结果。
