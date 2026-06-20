## UI 编写规则

- 跨平台 UI 工作以兼容性优先。修复 Linux/WSLg、macOS、Windows、Wayland/X11、DPI、字体、合成器或图形后端差异时，要保留已经正常工作的平台注册表象，并通过平台专用 QML 目录、带平台保护的 C++/QML 分支、资源别名或窄范围包装组件来增加兼容性。
- 不要为了单个平台问题重写共享 QML 组件，也不要替换已经正常工作的某个平台视觉路径，除非已经证明该 bug 会影响共享路径。Windows Acrylic/DWM 行为、macOS 行为以及其他已建立的平台路径必须保持不变，除非任务明确要求修改它们。
- 如果必须触碰共享 QML/C++，要保持现有默认视觉和行为兼容，然后为新的平台行为添加可选属性或平台专用包装器。在计划/复审中记录平台边界，并验证被修改的平台，以及任何共享路径被触碰的平台。
- 平台 UI 兼容性优先使用增量结构：`qml/linux`、`qml/windows`、`qml/macos`、`Qt.platform.os` 检查，或 `#ifdef Q_OS_*` 胶水代码。要在 `qml.qrc` 中显式注册平台资源；添加新兼容路径时，不要删除或替换旧的平台文件。
- 对于 WSLg/Linux 的玻璃效果和透明圆角窗口，避免使用未遮罩的 `ShaderEffectSource`/`MultiEffect`、整窗 `layer.*`、`QRegion` mask，以及依赖 `Rectangle.clip` 来裁剪圆角子元素。应使用 Linux 专用的抗锯齿 QML/Shape 外壳，带透明内边距，并且不要有方形背景板。

## 项目规则
- 不向后兼容：3.0 不兼容 2.0，开发阶段一律按最新协议/数据格式实现，旧客户端靠强制更新机制拦截升级，不靠代码兼容。改字段直接换新 key，不写"读新回退旧"的双读、不留旧字段别名、不写历史数据迁移读路径。删这类旧兼容前先按 `skills/no-backward-compat.md` 分类，避免误删平台/库适配、错误兜底、强制更新机制本身。
- 保持实现简洁，优先复用现有模式和 helper；不要为任务范围外的抽象、兼容层或顺手重构显著增加代码量。
- 代码风格语法尽可能现代，前面严格加上std::,不要使用using namespace std。
- 所有基础设施容器都在 Docker 中。
- Docker 容器必须保持稳定的发布端口。
- 项目工作必须使用 Docker 容器作为数据库、队列、对象存储、可观测性以及 AI/RAG 依赖。
- 修改代码、检查 MCP 或查看数据库时，先查找相关 Docker 容器或已配置的 MCP 工具。
- 新增或迁移的持久化测试统一放在仓库根 `tests/` 下，并按语言优先组织：C++ GTest 放 `tests/cpp/<主项目相对路径>/...`，Python pytest/unittest 放 `tests/python/<主项目相对路径>/...`，共享测试数据放 `tests/fixtures/...`。不要在 `apps/**/tests` 或其他业务模块内新增测试目录；历史服务内测试只能作为上下文读取，迁移时移动到上述结构。
- 默认项目工作现在面向 WSL/Linux，路径为 `/root/code/MemoChat-Qml-Drogon-linux`。
- Linux 依赖、缓存、模型以及大型生成产物优先下载到 `/data`。
- Arch 原生 Docker 是默认运行时。Docker 绑定数据使用 `/data/docker-data/memochat`。
- 仅在 Docker Desktop 迁移备份、旧版 Windows 脚本或明确的 Windows 客户端检查中使用 `D:`。
