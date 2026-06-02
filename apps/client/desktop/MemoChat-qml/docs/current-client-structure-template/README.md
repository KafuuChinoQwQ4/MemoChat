# Current Client Structure Template

这个模板描述的是当前 `MemoChat-qml` 客户端真实结构，不是推荐的重构目标结构。

它适合新人先理解现状：启动入口在哪里、QML 页面在哪里、C++ 控制器在哪里、资源怎么注册、业务链从哪里开始读。

## Current Tree

```text
apps/client/desktop/MemoChat-qml/
  CMakeLists.txt
  app/
    bootstrap/
      main.cpp
      MainQmlEngineSetup.cpp
      MainQmlTypeRegistry.cpp
      MainQmlWindowLoader.cpp
      MainPlatformBootstrap.cpp
      MainRuntimeConfig.cpp
    controller/
      AppController.h
      AppController*.cpp
      AppController*State.h
    session/
      AppSessionCoordinator.cpp
      SessionAuthCoordinator*.cpp
      SessionChatEntryCoordinator.cpp
      SessionRelationBootstrap.cpp
    connection/
      AppChatConnectionCoordinator.cpp
      AppChatConnectionPolicy.cpp
      AppControllerChatBootstrap.cpp
    coordinators/
      AppCoordinators.h
      ContactCoordinatorShell.cpp
      GroupCoordinator.cpp
      MediaCoordinator.cpp
      ProfileCoordinator.cpp
      CallCoordinator.cpp
      RegisterCountdownController.cpp
    window/
      MainWindowEffects.cpp
      MainWindowHooks.cpp
  qml/
    linux/
      Main.qml
      LoginPage.qml
      components/
    app/
      Main.qml
      ChatShellPage.qml
      ChatShellContent.qml
      AppWindowRuntime.js
      ChatShellRuntime.js
    auth/
      LoginPage.qml
      RegisterPage.qml
      ResetPage.qml
    chat/
      ChatSideBar.qml
      ChatLeftPanel.qml
      ChatConversationPane.qml
      ChatContactPane.qml
      ChatMorePane.qml
      ChatSettingsPane.qml
      contact/
      conversation/
      group/
      settings/
    agent/
    call/
    components/
    moments/
    pet/
    r18/
  features/
    auth/
      AuthController.h
      AuthController.cpp
      sources.cmake
    chat/
      controller/
      model/
      services/
      cache/
      sources.cmake
    contact/
      controller/
      model/
      sources.cmake
    call/
    profile/
    moments/
    agent/
    pet/
    r18/
    sources.cmake
  core/
    common/
    media/
    network/
    session/
    CMakeLists.txt
  shared/
    gateway/
      ClientGateway.h
      ClientGateway.cpp
      TransportEndpointPolicy.*
      TransportSelector.*
    media/
      LocalFilePickerService.*
      MediaUploadService.*
    ui/
      WindowShapeService.*
    utils/
      IconPathUtils.h
    sources.cmake
  live2d/
    assets/
    model/
    rendering/
    sources.cmake
  platform/
    windows/
    linux/
    macos/
    android/
    ios/
  resources/
    resources.cmake
    qrc/
      app-core.qrc
      qml-shell.qrc
      qml-chat.qrc
      qml-agent.qrc
      qml-moments.qrc
      qml-pet.qrc
      qml-r18.qrc
      icons.qrc
      web.qrc
    icons/
    app/
    web/
    live2d/
  docs/
```

## How The Current Client Starts

```text
app/bootstrap/main.cpp
  -> configureGateUrlPrefixes(...)
  -> registerMemoChatQmlTypes()
  -> AppController controller
  -> QQmlApplicationEngine engine
  -> configureMemoChatEngine(engine, controller)
  -> loadMemoChatMainWindow(engine, app, controller)
```

Important files:

- `app/bootstrap/main.cpp`: process startup, Qt style, WebEngine init, controller creation.
- `app/bootstrap/MainQmlEngineSetup.cpp`: injects global QML context values:
  - `controller`
  - `gateUrlPrefix`
  - `gateMediaUrlPrefix`
  - `livekitBridge`
- `app/bootstrap/MainQmlTypeRegistry.cpp`: registers enum/type names under `MemoChat 1.0`.
- `app/bootstrap/MainQmlWindowLoader.cpp`: chooses the QML entry:
  - Linux: `qrc:/qml/linux/Main.qml`
  - other platforms: `qrc:/qml/Main.qml`

## Current Layer Meaning

```text
qml/
```

UI pages and visual components. Most files bind to the global `controller` object. The shell is split between `qml/linux/Main.qml` for Linux/WSLg window behavior and `qml/app/*` for shared app page composition.

```text
app/controller/
```

The main QML-facing facade. `AppController.h` exposes many `Q_PROPERTY` and `Q_INVOKABLE` APIs. The implementation is split by behavior, for example auth, navigation, dialog state, group commands, media upload, message dispatch, and response handlers.

```text
features/
```

Business-specific C++ modules. These are not fully feature-first yet. Some modules contain controllers/models/services, while some behavior still lives in `AppController`.

```text
core/
```

Lower-level shared client foundation: common helpers, image/media primitives, network/transport, and user/session data.

```text
shared/
```

Cross-feature helper layer. `ClientGateway` is the current handle for HTTP, chat transport, message dispatcher, and user manager access.

```text
resources/qrc/
```

QML and asset registration. QML files are usually loaded through QRC aliases, so moving files requires updating these aliases carefully.

## Current Reading Route

For a newcomer, use this order:

1. Build entry:
   - `apps/client/desktop/MemoChat-qml/CMakeLists.txt`
   - `app/sources.cmake`
   - `features/sources.cmake`
   - `shared/sources.cmake`
   - `resources/resources.cmake`

2. Startup and QML injection:
   - `app/bootstrap/main.cpp`
   - `app/bootstrap/MainQmlEngineSetup.cpp`
   - `app/bootstrap/MainQmlWindowLoader.cpp`
   - `app/bootstrap/MainQmlTypeRegistry.cpp`

3. Window and shell:
   - `qml/linux/Main.qml`
   - `qml/app/Main.qml`
   - `qml/app/ChatShellPage.qml`
   - `qml/app/ChatShellContent.qml`

4. Main controller contract:
   - `app/controller/AppController.h`
   - `app/controller/AppControllerNavigation.cpp`
   - `app/controller/AppControllerAuth.cpp`
   - `app/controller/AppControllerMessageDispatch.cpp`
   - `app/controller/AppControllerMedia.cpp`

5. Feature-specific C++:
   - `features/auth/AuthController.*`
   - `features/chat/controller/ChatController.*`
   - `features/chat/model/ChatMessageModel.*`
   - `features/chat/services/*`
   - `features/agent/controller/AgentController*.cpp`

6. Shared transport and resources:
   - `shared/gateway/ClientGateway.*`
   - `core/network/*`
   - `resources/qrc/qml-shell.qrc`
   - `resources/qrc/qml-chat.qrc`

## Current Business Chains

### Login

```text
qml/auth/LoginPage.qml
  -> controller.login(...)
  -> app/controller/AppControllerAuth.cpp
  -> features/auth/AuthController.cpp
  -> shared/gateway/ClientGateway
  -> core/network/httpmgr.*
  -> app/controller/AppControllerAuthResponses.cpp
  -> app/session/SessionAuthCoordinator*.cpp
  -> controller.page = AppController.ChatPage
```

### Chat Message

```text
qml/chat/conversation/ChatComposerBar.qml
  -> qml/app/ChatShellContent.qml
  -> controller.sendCurrentComposerPayload(...)
  -> app/controller/AppControllerMedia.cpp
  -> app/controller/AppControllerMessageDispatch.cpp
  -> features/chat/controller/ChatController.cpp
  -> shared/gateway/ClientGateway
  -> core/network/IChatTransport or ChatMessageDispatcher
```

### Agent

```text
qml/agent/AgentPane.qml
  -> controller.agentController
  -> features/agent/controller/AgentController*.cpp
  -> features/agent/transport/AgentStreamClient.cpp
  -> shared/gateway/ClientGateway
```

## Notes For Future Refactor

- `AppController` is currently the central facade. It is convenient for QML but too broad for long-term feature ownership.
- `qml/` and `features/` are currently separated by file type/layer more than by feature ownership.
- Platform-specific Linux QML is important. Do not remove `qml/linux` or QRC aliases during refactor.
- Move QML files slower than C++ files. Keep old QRC aliases while changing implementation paths.
- Use one pilot module first, usually `auth`, before moving `chat` or `agent`.

