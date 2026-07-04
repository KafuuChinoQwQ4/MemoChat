# cxx_modules/ 目录树

> AIServer 的 GNU C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIService.cppm` | AIServer 模型列表成功判定和 agent task limit 归一化算法 module。 |
| `AIServiceClient.cppm` | AIServer AIOrchestrator HTTP 客户端使用的路径、URL 编码、SSE 和默认值算法 module。 |
| `AIServiceCore.cppm` | AIServer 核心 gRPC 服务使用的响应码、角色字面量、分页和会话校验算法 module。 |
| `AIServiceImpl.cppm` | AIServer gRPC service implementation 使用的 trace span 名称与 RPC 类型字面量 module。 |
| `AIServiceJsonDto.cppm` | AIServer JSON DTO 映射使用的对象/数组字段读取与默认模型选择算法 module。 |
| `AIServerRepository.cppm` | AIServer Postgres repository 使用的 schema 默认值、空结果与更新行数判断算法 module。 |
| `ConversationContext.cppm` | AIServer 对话上下文使用的角色字面量和消息窗口裁剪判定算法 module。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
