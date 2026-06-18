# api/ 目录树

> AI 编排服务的 HTTP 路由层，按能力拆分为多个 router。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 包初始化。 |
| `agent_router.py` | 智能体编排相关接口路由。 |
| `chat_router.py` | 对话/聊天接口路由。 |
| `kb_router.py` | 知识库（RAG）接口路由。 |
| `model_router.py` | 模型管理/查询接口路由。 |
| `pet_router.py` | AI 宠物相关接口路由。 |
| `recommend_router.py` | 推荐能力接口路由。 |
| `smart_router.py` | 智能分发/聚合入口路由。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
