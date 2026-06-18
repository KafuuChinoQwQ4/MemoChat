# view/ 目录树

> 聊天界面的 QML 视图层：主壳、左侧面板、会话窗格、AI 侧栏与模态层等。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`conversation/`](conversation/_TREE.md) | 会话窗格内的输入栏、消息列表与气泡等组件 |
| [`sidebar/`](sidebar/_TREE.md) | 左侧对话列表、表头与加好友/加群弹窗 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatAgentSidePane.qml` | AI 助手侧边窗格，含会话/房间列表、删除与会话重命名弹窗 |
| `ChatConversationPane.qml` | 会话主窗格（消息流 + 输入） |
| `ChatLeftPanel.qml` | 左侧面板容器，负责账号态 Live2D 入口数据装配 |
| `ChatLive2DEntryPane.qml` | Live2D 入口窗格，展示角色导入状态 |
| `ChatModalLayer.qml` | 聊天页弹窗/模态叠加层 |
| `ChatNormalFace.qml` | 常规聊天主界面布局（行布局） |
| `ChatShellContent.qml` | 聊天页整体外壳内容容器 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
