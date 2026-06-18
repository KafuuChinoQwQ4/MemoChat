# proto/ 目录树

> 跨服务共享的 protobuf/gRPC 接口定义：客户端聊天/好友/群消息协议、ChatServer 内部 gRPC 契约，以及 AI 消息协议。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ai_message.proto` | AI 聊天/会话/历史/知识库等消息定义 |
| `chat_internal.proto` | ChatServer 内部 gRPC 服务（关系/私聊/群聊内部 RPC） |
| `message.proto` | 客户端面向的聊天与验证码服务协议（VarifyService/ChatService 等） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
