# domain/ 目录树

> ChatServer 领域核心，按业务能力分层：消息收发、好友关系、会话、在线投递、编排与用户管理，并通过 ports 定义所有跨层依赖接口（六边形架构端口）。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`delivery/`](delivery/_TREE.md) | 投递运行时、任务分发与投递任务载荷 DTO |
| [`message/`](message/_TREE.md) | 私聊/群聊消息服务与消息载荷 |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 领域层共享轻量算法的 C++ module 接口分组 |
| [`orchestration/`](orchestration/_TREE.md) | 逻辑系统与处理器注册（编排） |
| [`ports/`](ports/_TREE.md) | 领域依赖的端口接口抽象 |
| [`relation/`](relation/_TREE.md) | 好友关系服务与内部 gRPC |
| [`session/`](session/_TREE.md) | 聊天会话服务 |
| [`users/`](users/_TREE.md) | 在线用户管理与用户支撑 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatGroupCommandDtos.h` | Chat 群组简单命令 request DTO 与兼容字段提取 helper |
| `ChatHistoryCommandDtos.h` | Chat 历史消息命令 request DTO 与游标字段提取 helper |
| `ChatHistoryOutputDtos.h` | Chat 历史消息与离线推送稳定输出 DTO 及兼容 JSON helper |
| `ChatMessageCommandDtos.h` | Chat 私聊/群聊消息命令 request/response 与稳定通知 root DTO 及兼容 JSON helper |
| `ChatRelationCommandDtos.h` | Chat 关系命令 request/response、通知 root 与内部事件 payload DTO 及兼容 JSON helper |
| `ChatRelationGroupDtos.h` | Chat 关系/群列表稳定输出 row DTO 与 JSON 转换 helper |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
