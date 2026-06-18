# AIGatewayService/ 目录树

> AI 网关微服务：作为 Gate 与后端 AI 能力之间的 HTTP 接入层，负责把 AI 相关路由聚合并转发到 AIServer/AIOrchestrator。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | 服务进程入口（main/server 启动）。 |
| [`domain/`](domain/_TREE.md) | AI 网关领域逻辑：路由模块、AI 服务客户端与配置档案。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | AIGatewayService 的构建目标定义。 |
| `aigateway.ini` | 服务运行时配置（端口、上游地址等）。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
