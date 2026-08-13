# bootstrap/ 目录树

> 进程启动引导层，负责 main 入口、日志初始化、平台引导、QML 引擎装配与类型注册以及运行时配置解析。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MainLogging.cpp` | 日志配置、敏感字段脱敏及结构化文件/控制台输出实现 |
| `MainLogging.h` | 日志初始化、脱敏与 Qt 消息处理接口声明 |
| `MainPlatformBootstrap.cpp` | 平台相关启动引导实现 |
| `MainPlatformBootstrap.h` | 平台启动引导接口声明 |
| `MainQmlBootstrap.h` | QML 引导接口：类型注册、引擎配置、主窗口加载 |
| `MainQmlEngineSetup.cpp` | QML 引擎配置实现 |
| `MainQmlTypeRegistry.cpp` | QML 自定义类型注册实现 |
| `MainQmlWindowLoader.cpp` | 主窗口 QML 加载实现 |
| `MainRuntimeConfig.cpp` | 运行时配置解析、Release HTTPS 前置校验与网关 URL 前缀配置实现 |
| `MainRuntimeConfig.h` | 运行时配置接口声明 |
| `main.cpp` | 应用程序入口与 Linux 可分发构建策略标记 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
