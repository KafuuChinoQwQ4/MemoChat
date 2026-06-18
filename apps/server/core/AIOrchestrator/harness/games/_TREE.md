# games/ 目录树

> A2A 多智能体游戏子系统：房间生命周期、规则引擎、参与者与基于 tick 的推进图。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 包初始化。 |
| `agent_actions.py` | 智能体在游戏中的动作定义。 |
| `contracts.py` | 游戏状态/事件/参与者等数据契约。 |
| `host.py` | 游戏主持逻辑：驱动事件与状态。 |
| `participants.py` | 参与者管理。 |
| `presets.py` | 预置游戏模板。 |
| `room_lifecycle.py` | 房间创建/加入/结束等生命周期管理。 |
| `rules.py` | 游戏规则引擎。 |
| `service.py` | A2A 游戏服务对外入口。 |
| `store.py` | 游戏状态存储。 |
| `tick_graph.py` | 基于 LangGraph 的逐 tick 推进图。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
