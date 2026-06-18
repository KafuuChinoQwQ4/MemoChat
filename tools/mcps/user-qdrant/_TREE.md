# user-qdrant/ 目录树

> Qdrant 向量数据库的用户级 MCP server，向 AI 助手暴露向量检索与维护能力。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`tools/`](tools/_TREE.md) | 该 MCP server 暴露的 Qdrant 操作工具定义 JSON |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `SERVER_METADATA.json` | MCP server 标识与元信息（serverName=qdrant） |
| `user_qdrant_mcp_server.py` | Qdrant MCP server 进程入口，转发向量检索/写入请求 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
