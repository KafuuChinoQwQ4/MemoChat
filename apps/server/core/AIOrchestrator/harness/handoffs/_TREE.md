# handoffs/ 目录树

> harness 的任务移交层，使用 LangGraph 编排智能体之间的消息流与移交步骤。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 包初始化。 |
| `flow_graph.py` | 基于 LangGraph 的高层 handoff flow 节点流转图。 |
| `service.py` | 移交服务：提供 flow 模板、图节点实现与运行结果组装。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
