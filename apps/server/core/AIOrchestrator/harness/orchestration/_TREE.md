# orchestration/ 目录树

> harness 的编排层，负责智能体执行的规划与调度。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `README.md` | 编排层说明文档。 |
| `__init__.py` | 包初始化。 |
| `agent_graph.py` | 基于 LangGraph 的 agent 节点流转图，负责 preparation/completion/finalization 的控制流。 |
| `agent_service.py` | 智能体编排服务：组织各层执行一次完整运行。 |
| `planner.py` | 规划策略：根据技能与记忆生成执行计划。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
