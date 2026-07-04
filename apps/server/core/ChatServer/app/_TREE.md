# app/ 目录树

> ChatServer 集群各微服务进程的 main 入口与运行时装配：接入、消息、关系、关系查询、投递 worker，以及共享的 ChatRuntime 启动骨架。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 共享运行时使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatDeliveryWorker.cpp` | 投递 worker 进程入口，消费投递任务 |
| `ChatMessageService.cpp` | 消息服务进程入口，承载私聊/群聊内部 gRPC |
| `ChatRelationQueryService.cpp` | 关系查询服务进程入口（只读关系查询） |
| `ChatRelationServiceWorker.cpp` | 关系服务 worker 进程入口（关系写入 + 事件） |
| `ChatRuntime.cpp` | 各服务共享的运行时启动/装配实现 |
| `ChatRuntime.h` | 运行时装配声明 |
| `ChatServer.cpp` | 接入服务进程入口（TCP/QUIC 接入） |
| `ChatServerCore.cpp` | 聚合链接占位，确保核心目标被链接 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
