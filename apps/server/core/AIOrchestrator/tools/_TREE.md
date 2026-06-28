# tools/ 目录树

> 智能体可调用的工具集合及注册表，含本地工具与 MCP 桥接。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 包初始化。 |
| `calculator_tool.py` | 计算器工具。 |
| `knowledge_base_tool.py` | 知识库查询工具。 |
| `mcp_bridge.py` | MCP 桥接：经 subprocess+stdio+JSON-RPC 调用外部 MCP Server。 |
| `registry.py` | 工具注册表：登记所有可用工具（含 MCP）。 |
| `translator_tool.py` | 翻译工具。 |
| `web_search_tool.py` | 网络搜索工具。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
