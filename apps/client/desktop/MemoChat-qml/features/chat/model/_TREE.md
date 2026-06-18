# model/ 目录树

> 暴露给 QML 的聊天消息列表模型及其内容/查询/变更/呈现分片，以及聊天 UI 状态。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatMessageModel.cpp` | 消息列表模型 QAbstractListModel 核心实现 |
| `ChatMessageModel.h` | ChatMessageModel 类声明与角色定义 |
| `ChatMessageModelContent.cpp` | 消息内容（正文/附件）处理实现片段 |
| `ChatMessageModelMutations.cpp` | 消息增删改等变更操作片段 |
| `ChatMessageModelPresentation.cpp` | 消息呈现/格式化相关片段 |
| `ChatMessageModelQueries.cpp` | 消息查询/检索相关片段 |
| `ChatUiState.h` | 聊天界面 UI 状态结构 ChatUiState |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
