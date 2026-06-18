# composition/ 目录树

> 应用依赖组合根，注册各功能模块、装配端口与信号绑定，把控制器、协调器、网关连成完整运行时。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AppAuthFeaturePortBinder.cpp` | 鉴权功能端口绑定 |
| `AppCallPortBinder.cpp` | 通话端口绑定 |
| `AppCallSignalBinder.cpp` | 通话信号绑定 |
| `AppChatDispatcherRouterFactory.cpp` | 聊天分发事件路由工厂实现 |
| `AppChatDispatcherRouterFactory.h` | 聊天分发事件路由工厂声明 |
| `AppChatDispatcherSignalBinder.cpp` | 聊天分发信号绑定 |
| `AppChatDispatcherSignalRoutes.cpp` | 聊天分发信号路由表实现 |
| `AppChatDispatcherSignalRoutes.h` | 聊天分发信号路由表声明 |
| `AppChatTransportSignalBinder.cpp` | 聊天传输层信号绑定 |
| `AppComposition.cpp` | 组合根实现，创建并持有各功能模块与控制器 |
| `AppComposition.h` | 组合根接口声明 |
| `AppConnectionPortBinder.cpp` | 连接端口绑定 |
| `AppContactFeaturePortBinder.cpp` | 联系人功能端口绑定 |
| `AppFeaturePortBinder.cpp` | 通用功能端口绑定 |
| `AppFeatureRegistry.cpp` | 功能模块注册表实现 |
| `AppFeatureRegistry.h` | 功能模块注册表声明，聚合各 controller/model |
| `AppGroupFeaturePortBinder.cpp` | 群组功能端口绑定 |
| `AppHttpSignalBinder.cpp` | HTTP 信号绑定 |
| `AppMediaPortBinder.cpp` | 媒体端口绑定 |
| `AppPortBinder.cpp` | 端口绑定实现（AppController 私有方法） |
| `AppPortBinder.h` | 端口绑定声明占位 |
| `AppPortRegistry.cpp` | 端口注册表实现 |
| `AppPortRegistry.h` | 端口注册表声明，汇集各协调器/状态引用 |
| `AppPostLoginBootstrapPortBinder.cpp` | 登录后引导端口绑定 |
| `AppProfileFeaturePortBinder.cpp` | 个人资料功能端口绑定 |
| `AppRegisterCountdownPortBinder.cpp` | 注册倒计时端口绑定 |
| `AppRelationBootstrapPortBinder.cpp` | 关系引导端口绑定 |
| `AppSessionAuthPortBinder.cpp` | 会话鉴权端口绑定 |
| `AppSessionLogoutPortBinder.cpp` | 会话登出端口绑定 |
| `AppSessionPortBinder.cpp` | 会话端口绑定 |
| `AppShellSignalBinder.cpp` | 外壳信号绑定 |
| `AppSignalBinder.cpp` | 信号绑定实现（AppController 私有方法） |
| `AppSignalBinder.h` | 信号绑定声明占位 |
| `AppTimerSignalBinder.cpp` | 定时器信号绑定 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
