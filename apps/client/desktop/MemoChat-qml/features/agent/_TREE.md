# agent/ 目录树

> AI 智能体功能模块，涵盖对话、多模型协作、记忆/知识库与文字游戏，按分层组织。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`controller/`](controller/_TREE.md) | 智能体业务控制器（对话/会话/记忆/知识库/流式等） |
| [`game/`](game/_TREE.md) | 文字游戏客户端与房间/模板逻辑 |
| [`model/`](model/_TREE.md) | 智能体消息模型与请求类型定义 |
| [`resources/`](resources/_TREE.md) | 智能体模块 QML 资源清单 |
| [`runtime/`](runtime/_TREE.md) | 智能体界面运行时 JS 逻辑 |
| [`transport/`](transport/_TREE.md) | 智能体网络请求与流式传输客户端 |
| [`view/`](view/_TREE.md) | 智能体相关 QML 界面 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `sources.cmake` | 汇总 agent 模块源码与资源到构建系统 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
