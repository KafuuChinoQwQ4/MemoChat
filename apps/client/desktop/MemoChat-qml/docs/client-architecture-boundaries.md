# MemoChat QML Client Architecture Boundaries

本文件记录当前客户端模块化后的边界。它不是历史结构说明，而是后续改动需要遵守的防回退契约：`AppController` 只能做 app shell 和大入口调度，功能模块要在自己的 controller/service/coordinator 内完成状态循环。

## Target Shape

```text
app/bootstrap
  -> 创建 QApplication / QQmlApplicationEngine / AppController
  -> 注册 QML 类型、资源、上下文对象

app/controller/AppController
  -> 只保留应用壳、页面切换、跨模块入口和少量兼容信号
  -> 不重新承接 chat/contact/group/profile/call 的业务循环

app/composition/AppPortRegistry
  -> 组合根和端口绑定层
  -> 把 AppController/AppSessionCoordinator/feature controllers 通过窄 port 连接

app/session, app/connection, app/coordinators
  -> 登录会话、聊天连接、媒体、通话等 app 级协调器
  -> 允许做跨模块胶水，但不拥有 feature 内部业务状态机

features/*
  -> 各功能的 controller/viewmodel/model/service/cache
  -> 内部请求、响应、事件、分页、选择、草稿、已读、历史和发送循环在模块内闭合

qml/linux, qml/app, features/*/view
  -> 只表达窗口 shell、页面组合和用户交互
  -> 不写业务编排，不绕过 feature facade 直接操作跨模块状态
```

## AppController

`AppController` 是应用壳，不是功能中枢。

允许保留：

- 创建和暴露顶层 facade：`shellViewModel()`、`authViewModel()`、`chatViewModel()`、`contactController()`、`groupController()`、`profileController()`、`callController()`。
- 页面和 tab 级导航入口，例如 login/register/reset/chat page、chat/contact/settings/moments/agent/live2d tab。
- 登录后 bootstrap 的大入口，以及需要跨 session、connection、feature 的顺序调度。
- 少量向旧 QML 或旧信号兼容的 change signals，但不能继续扩大。
- `AppChat*Binding.cpp` 这类绑定文件，用来把 app shell 状态投影给 chat feature，或把 chat feature intent 接回 app 级能力。

禁止回退：

- 不新增 `Q_PROPERTY` 或 `Q_INVOKABLE` 作为 QML 直接业务入口。
- 不把私聊发送、群聊发送、历史分页、已读、草稿、置顶、免打扰、附件发送、消息变更等循环重新写回 `AppController`。
- 不在 `AppController` 里直接解析 feature payload，然后再手动更新多个 feature model。
- 不把 contact/group/profile/call 的请求构造、响应解析、事件 effect 重新集中到 app controller。

当前防线由这些测试覆盖：

- `tests/python/apps/client/desktop/MemoChat-qml/app/controller/test_appcontroller_facade_pruning_contract.py`
- `tests/python/apps/client/desktop/MemoChat-qml/app/controller/test_appcontroller_shell_guardrails.py`
- `tests/python/apps/client/desktop/MemoChat-qml/app/controller/test_direct_feature_context_contract.py`

## AppPortRegistry

`AppPortRegistry` 是 composition root。它的职责是绑定端口，不是持有新的业务规则。

允许保留：

- 创建 `AuthCommandPort`、`CallCommandPort`、chat projection/send/history/media/read/group ports。
- 把 `AppSessionCoordinator`、`MediaCoordinator`、`CallCoordinator`、`ChatFeatureController` 和各 feature controller 用函数对象接起来。
- 读取 app shell 快照，然后交给 feature controller/service 做决策。

禁止回退：

- 不在 port binder 内直接实现完整业务流程。
- 不让一个 port context 暴露“全量 AppController 能力”。每个 port 只暴露该 workflow 需要的 snapshot、command、effect。
- 不通过 `AppPortRegistry` 绕过 feature controller 直接写 feature model，除非这是明确的跨模块投影或兼容桥。

建议继续保持的文件形态：

- session/auth/logout/connection 绑定放在 `app/composition/App*PortBinder.cpp`。
- chat 绑定放在 `app/controller/AppChat*Binding.cpp`，但每个文件只承接一个 workflow。
- 复杂依赖包用 factory，例如 `ChatEventDependenciesFactory`、`PrivateHistoryDependenciesFactory`、`IncomingMessageRouterFactory`。

## ChatFeatureController

`ChatFeatureController` 是 chat 功能入口，不应该成为新的大中枢。

它可以负责：

- 连接 `ChatViewModel` 与 chat services。
- 维护当前 dialog runtime：选择、草稿、置顶、免打扰、pending reply、pending attachments。
- 调用私聊、群聊、历史、消息 mutation、上传附件分发等服务。
- 对外暴露 chat feature 需要的窄 API，供 app shell 绑定层调用。

它不应该负责：

- 直接访问 GateServer/ChatServer 之外的跨功能业务规则。
- 复制 contact/group/profile/call 的业务逻辑。
- 在单个 `.cpp` 中继续堆叠所有 chat runtime。新的 chat workflow 要落到对应服务或 shard：
  - dialog runtime：`ChatFeatureControllerDialog*.cpp`
  - private conversation/history/send：`PrivateChat*Service` 和 `ChatFeatureControllerPrivate*.cpp`
  - group conversation/history/mutation：`GroupConversationService` 及其分片
  - incoming routing：`IncomingMessageRouter`
  - message mutation：`MessageMutationCommandService`
  - uploaded attachment dispatch：`UploadedAttachmentDispatchService`

当前防线由这些测试覆盖：

- `tests/cpp/apps/client/desktop/MemoChat-qml/features/chat/controller/`
- `tests/cpp/apps/client/desktop/MemoChat-qml/features/chat/services/`
- `tests/python/apps/client/desktop/MemoChat-qml/features/chat/test_chat_qml_surface_contract.py`

## Feature Controllers

各功能 controller 是该功能的入口 facade。它们应当自己闭合请求、响应、事件和 UI-facing state。

`features/auth`

- `AuthViewModel` 承接 QML 登录/注册/重置命令。
- 凭据缓存、表单状态、倒计时和 auth command 不回流到 `AppController` 的 QML surface。

`features/contact`

- 联系人搜索、申请、同意、删除、事件 effect 归 `ContactController` / `ContactEventService`。
- `AppController` 只做“当前选中联系人跳转聊天”等跨模块入口。

`features/group`

- 建群、群管理响应、群事件 effect、错误转换归 group controller/services。
- chat 内群会话消息循环归 chat feature 的 `GroupConversationService`，不要和群管理 controller 混在一起。

`features/profile`

- 个人资料请求 payload、响应应用、头像/昵称等 profile state 归 `ProfileController`。
- app shell 只同步当前用户投影。

`features/call`

- 通话命令、LiveKit 事件、HTTP payload 和 bridge state 归 `CallController` / `CallCoordinator`。
- QML 只触发 `CallController` facade，不直接调 AppController 旧命令。

## Services And Coordinators

services/coordinators 是把业务循环从 controller 中拿走的主要承载层。

放在 service 的内容：

- payload 构造、响应解析、历史分页、ack/read/mutation 规则。
- cache store 读写和数据归一化。
- 可被 GTest 覆盖的纯业务或低副作用逻辑。

放在 coordinator 的内容：

- app 级生命周期、跨 feature 的异步串联、网络/媒体/通话 bridge。
- session bootstrap、chat connection、media upload queue、LiveKit bridge 回调。

不放在 service/coordinator 的内容：

- QML 视觉状态。
- app shell 页面和全局 tab 决策。
- 不必要的跨模块对象引用。

## QML Shell

QML shell 只做视觉和交互入口。

允许：

- `qml/linux/Main.qml` 保留 Linux/WSLg 窗口、透明圆角、平台外壳差异。
- `qml/app/Main.qml` 和 `qml/app/ChatShellPage.qml` 组合页面、绑定 facade、转发用户 intent。
- `features/*/view` 内部消费对应 feature facade。

禁止：

- 在 QML 里直接拼业务 payload。
- 在 shell QML 里直接维护私聊/群聊历史、已读、分页、会话选择等业务状态。
- 为了 Linux 修复重写共享 QML 视觉路径；Linux 平台差异继续走 `qml/linux` 或平台专用 wrapper。

当前防线由这些测试覆盖：

- `tests/python/apps/client/desktop/MemoChat-qml/qml/app/test_main_window_runtime_contract.py`
- `tests/python/apps/client/desktop/MemoChat-qml/features/chat/test_chat_qml_surface_contract.py`
- `tests/python/apps/client/desktop/MemoChat-qml/qml/test_qml_scroll_window_handoff.py`

## Change Rules

后续改客户端时按这个顺序判断放置位置：

1. 这是单个 feature 内的 workflow 吗？放到 `features/<name>/controller|services|model|viewmodel`。
2. 这是跨 feature 的生命周期或网络 bridge 吗？放到 `app/session`、`app/connection` 或 `app/coordinators`。
3. 这是端口连接和依赖注入吗？放到 `app/composition` 或窄 factory。
4. 这是 QML 页面组合吗？放到 `qml/app` 或 `features/*/view`。
5. 只有必须被 app shell 统一入口调度的内容，才留在 `AppController`。

新增功能必须同时补一条边界测试或扩展既有边界测试，证明业务入口没有回退到 `AppController`。
