# chat/ 目录树

> 桌面端聊天 feature 模块：私聊/群聊会话、消息模型、对话列表、缓存、传输 service 与 QML 视图的整体聚合。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cache/`](cache/_TREE.md) | 私聊/群聊消息的本地缓存存储与编解码 |
| [`controller/`](controller/_TREE.md) | 聊天 feature 控制器及其按职责拆分的实现片段 |
| [`model/`](model/_TREE.md) | 暴露给 QML 的消息列表模型与 UI 状态 |
| [`resources/`](resources/_TREE.md) | 聊天模块 QML/JS 资源清单（qrc） |
| [`runtime/`](runtime/_TREE.md) | 会话面板的纯 JS 运行时辅助逻辑 |
| [`services/`](services/_TREE.md) | 私聊/群聊收发、历史、对话列表、协议路由等服务层 |
| [`view/`](view/_TREE.md) | 聊天主壳与各窗格的 QML 视图 |
| [`viewmodel/`](viewmodel/_TREE.md) | 连接控制器与视图的聊天 ViewModel |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `sources.cmake` | 汇总本 feature 的 C++ 源文件、头文件与资源到构建清单 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
