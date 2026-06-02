# Feature Module Template

这个目录只是客户端重构外形模板，不参与当前 `MemoChatQml` 构建。

目标不是把所有文件按 `model/`、`view/`、`controller/` 三个全局目录横向拆开，而是按业务模块纵向聚合。新人读登录、聊天、Agent 这类流程时，先进入对应 feature，再在 feature 内看 UI、状态、业务调用和数据模型。

## Target Shape

```text
MemoChat-qml/
  app/
    bootstrap/                 # main, QML engine, type registration
    shell/                     # global app shell, page routing
    window/                    # platform window hooks and visual effects
  features/
    auth/
      view/                    # QML pages and visual-only components
      viewmodel/               # QObject exposed to QML
      model/                   # DTOs, state structs, QAbstractListModel
      service/                 # HTTP/transport calls through shared gateway
      resources/               # module qrc aliases
      sources.cmake
    chat/
      view/
      viewmodel/
      model/
      service/
      cache/
      resources/
      sources.cmake
    agent/
    contact/
    moments/
    pet/
  shared/                      # ClientGateway, media upload, shared UI/utils
  core/                        # lower-level network/session/common pieces
  platform/                    # Windows/Linux/macOS compatibility glue
  resources/                   # global icons, app assets, web, Live2D bundles
```

## Boundary Rules

- `view/`: QML only. It emits signals and binds to properties. It should not know HTTP paths, transport details, or persistence details.
- `viewmodel/`: the QML-facing QObject. It owns `Q_PROPERTY`, `Q_INVOKABLE`, loading/error state, validation results, and page-level commands.
- `model/`: plain data, DTOs, enum/state structs, and Qt item models.
- `service/`: network and business calls. It may depend on `ClientGateway`, `HttpMgr`, `IChatTransport`, and parser helpers. It should not depend on QML.
- `app/`: global lifecycle only. It creates controllers/viewmodels, injects them into QML, and handles cross-feature routing.
- `shared/`: only code reused by two or more features.

## Migration Strategy

Start with `auth` as a pilot instead of moving the whole client at once.

1. Create `features/auth/viewmodel/AuthViewModel.*` and let it wrap the existing `AuthController` behavior.
2. Keep old QML aliases in QRC first, so existing imports still work.
3. Let `AppController` keep forwarding old methods during migration.
4. Update QML gradually from `controller.login(...)` to `auth.login(...)`.
5. After auth is stable, repeat the same shape for `chat`, then `agent`.

The important compatibility rule is: aliases and public QML names should move slower than files. This avoids breaking `qml/linux/Main.qml`, shared shell QML, and platform-specific imports during the directory migration.

