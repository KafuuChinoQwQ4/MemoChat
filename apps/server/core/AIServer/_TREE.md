# AIServer/ 目录树

> C++ 实现的 AI 服务节点：承接网关转发的 AI 请求，维护会话上下文并对接 AIOrchestrator/LLM，落库会话与智能日志。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`db/`](db/_TREE.md) | AI 会话与智能日志的数据库仓储。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIServer.cpp` | 服务进程入口：初始化并启动 AIServer。 |
| `AIServiceClient.cpp` | 调用下游 AI 编排/模型服务的客户端实现。 |
| `AIServiceClient.h` | AI 服务客户端接口声明。 |
| `AIServiceCore.cpp` | AI 服务核心逻辑实现。 |
| `AIServiceCore.h` | AI 服务核心接口声明。 |
| `AIServiceImpl.cpp` | AI 服务的具体业务实现。 |
| `AIServiceImpl.h` | AI 服务实现接口声明。 |
| `AIServiceJsonMapper.cpp` | AI 服务请求/响应与 JSON 的映射实现。 |
| `AIServiceJsonMapper.h` | JSON 映射接口声明。 |
| `CMakeLists.txt` | AIServer 构建目标定义。 |
| `ConfigMgr.cpp` | 配置管理实现。 |
| `ConfigMgr.h` | 配置管理接口声明。 |
| `ConversationContext.cpp` | 对话上下文管理实现。 |
| `ConversationContext.h` | 对话上下文接口声明。 |
| `config.ini` | 服务运行时配置。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
