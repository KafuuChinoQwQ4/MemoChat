# domain/ 目录树

> ChatServer 领域核心，按业务能力分层：消息收发、好友关系、会话、在线投递、编排与用户管理，并通过 ports 定义所有跨层依赖接口（六边形架构端口）。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`delivery/`](delivery/_TREE.md) | 投递运行时与任务分发 |
| [`message/`](message/_TREE.md) | 私聊/群聊消息服务与投递载荷 |
| [`orchestration/`](orchestration/_TREE.md) | 逻辑系统与处理器注册（编排） |
| [`ports/`](ports/_TREE.md) | 领域依赖的端口接口抽象 |
| [`relation/`](relation/_TREE.md) | 好友关系服务与内部 gRPC |
| [`session/`](session/_TREE.md) | 聊天会话服务 |
| [`users/`](users/_TREE.md) | 在线用户管理与用户支撑 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
