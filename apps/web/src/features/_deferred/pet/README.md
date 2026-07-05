# _deferred/pet — 桌面宠物（推迟）

v1 不实现。以下是扩展位说明。

## 说明

桌面悬浮宠物（Live2D 渲染 + Windows 窗口桥）是原生能力，无法直接移植到浏览器。
Web 版的后续方案：

1. 将 Live2D 渲染迁移到 `<canvas>` (pixi-live2d-display)
2. 移除 Windows 原生窗口桥依赖
3. 作为侧边栏或 overlay 组件嵌入网页
4. 摄像头视觉 / 语音 greeting 功能通过 WebRTC + Web Audio API 实现

## 桌面端参考

- `features/pet/` in `apps/client/desktop/MemoChat-qml/`
- Live2D runtime: `apps/client/desktop/MemoChat-qml/live2d/`
