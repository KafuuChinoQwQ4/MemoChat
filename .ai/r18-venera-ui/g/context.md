# R18 Sidebar Icon Navigation Context

Task date: 2026-05-02

User request: 把 R18 页面主页/搜索等按钮放在最左边侧边栏，使用 `apps/client/desktop/MemoChat-qml/src` 新增的四张图标，从上到下为主页、本地、导航、数据源；修正 R18 界面字体为灰黑；删除没用的旧侧边栏。

Relevant files:
- apps/client/desktop/MemoChat-qml/qml.qrc
- apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml
- apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml

Icon resources added:
- qrc:/icons/r18_home.png -> src/主页.png
- qrc:/icons/r18_local.png -> src/本地素材备份.png
- qrc:/icons/r18_navigation.png -> src/导航.png
- qrc:/icons/r18_datasource.png -> src/源数据分析.png

Implementation notes:
- The R18 face in ChatShellPage now owns the leftmost R18 navigation rail.
- The old ChatSideBar inside the R18 face was removed.
- The old Canvas-based navigation rail inside R18ShellPane was removed.
- R18ShellPane now only renders the content glass surface and exposes/syncs `viewMode` with ChatShellPage.
- Text tokens changed to gray/dark colors for transparent white glass readability.
