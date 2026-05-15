---
description: 为 MemoChat QML 和 ops UI 创建或改造 SVG/icon 资产；除非用户明确提供外部矢量化服务，否则不使用它们。
---

# MemoChat 图标

用于项目 UI 图标工作，覆盖 QML/client/ops 界面。

## 输入

收集：

- 目标界面：MemoChat QML 客户端、MemoOps QML、web/ops 资产或文档
- 预期含义和状态
- 目标尺寸
- 颜色行为：固定颜色、主题颜色或 mask/tint
- 目标路径
- 如果提供了截图或源资产路径，也一并记录

如果用户粘贴了图片但没有文件路径，先总结视觉需求；只有在需要精确描摹时才询问已保存文件路径。

## 搜索现有资产

创建任何内容前，检查：

- `apps/client/desktop/MemoChat-qml`
- `apps/client/desktop/MemoChatShared`
- `infra/Memo_ops/client/MemoOps-qml`
- 附近的 `assets`、`icons`、`resources`、`qml.qrc` 或 CMake resource entries

优先采用现有风格、命名、尺寸和资源注册模式。

## SVG 规则

- 保持 SVG 简洁且可手工编辑。
- 正确使用 `viewBox`；简单坐标可行时不要依赖任意 transform。
- 当 QML 组件会给图标 tint 时，优先使用 `currentColor` 或单一 fill。
- 除非设计需要，否则避免嵌入 raster image。
- 避免 metadata、comments、编辑器特有属性和巨大的 path 噪音。
- 文件名使用小写加下划线，或遵循现有本地约定。

## 实现

1. 在正确资源目录中创建或编辑 SVG。
2. 如果项目使用 QML 资源注册，更新该注册。
3. 更新引用图标的 QML 组件或样式文件。
4. 对有状态图标，验证 normal/hover/pressed/disabled 颜色遵循现有 QML 模式。
5. 只有有帮助时，才把工作迭代放到 `.ai/icon-<name>/` 下。

## 验证

使用最窄相关检查：

- 直接检查 SVG
- 当资源注册或 QML 引用变化时，运行客户端构建：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

对于视觉工作，如果存在本地路径/脚本，运行或截图相关 QML 界面。除非必要，否则不要发明新的 renderer。

## 完成

报告：

- 最终资产路径
- 为注册/使用它而更新的文件
- 执行过的构建或视觉检查
- 仍需的任何手工视觉验证
