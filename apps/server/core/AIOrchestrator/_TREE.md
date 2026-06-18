# AIOrchestrator/ 目录树

> Python 实现的 AI 编排服务：以 LangGraph/LLM 为核心，提供智能体编排、RAG 检索、推荐、宠物（pet）等 AI 能力，对外暴露 HTTP API。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`api/`](api/_TREE.md) | 各能力的 HTTP 路由（agent/chat/kb/model/pet 等）。 |
| [`db/`](db/_TREE.md) | 关系库（Postgres）访问客户端。 |
| [`docs/`](docs/_TREE.md) | 编排引擎与 MCP 工具的工程文档。 |
| [`graph/`](graph/_TREE.md) | Neo4j 图数据库客户端与推荐逻辑。 |
| [`harness/`](harness/_TREE.md) | 智能体运行框架（分层：编排/记忆/工具/守护/游戏/宠物等）。 |
| [`llm/`](llm/_TREE.md) | 多家 LLM 提供方适配与统一管理。 |
| [`observability/`](observability/_TREE.md) | 指标、追踪与 LangSmith 埋点。 |
| [`rag/`](rag/_TREE.md) | 检索增强生成链（文档处理/嵌入/检索）。 |
| [`schemas/`](schemas/_TREE.md) | API 请求/响应的数据模型。 |
| [`tools/`](tools/_TREE.md) | 智能体可调用的工具集合与注册表。 |
| [`utils/`](utils/_TREE.md) | 通用工具（日志等）。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `Dockerfile` | 编排服务镜像构建定义。 |
| `.dockerignore` | Docker 构建上下文忽略规则。 |
| `__init__.py` | 包初始化。 |
| `config.py` | 全局配置（settings）加载与定义。 |
| `config.yaml` | 配置默认值文件。 |
| `docker-compose.yml` | 本地依赖（DB/向量库等）编排。 |
| `main.py` | 服务入口：装配路由并启动 HTTP 应用。 |
| `requirements-ci.txt` | CI 环境精简依赖清单。 |
| `requirements.txt` | 运行依赖清单。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
