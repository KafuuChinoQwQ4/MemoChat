# harness/ 目录树

> 智能体运行框架：按分层（编排、记忆、工具执行、守护、移交、互操作、游戏、宠物、运行时等）组织的可组合 agent runtime，定义统一契约与端口。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cache/`](cache/_TREE.md) | 语义缓存服务。 |
| [`evals/`](evals/_TREE.md) | 智能体/RAG 评测服务与用例。 |
| [`execution/`](execution/_TREE.md) | 工具执行器。 |
| [`feedback/`](feedback/_TREE.md) | 反馈评估与运行轨迹存储。 |
| [`games/`](games/_TREE.md) | A2A 多智能体游戏房间与规则引擎。 |
| [`guardrails/`](guardrails/_TREE.md) | 输入/输出安全守护检查。 |
| [`handoffs/`](handoffs/_TREE.md) | 智能体间任务移交编排。 |
| [`interop/`](interop/_TREE.md) | 智能体互操作（AgentCard 等）。 |
| [`knowledge/`](knowledge/_TREE.md) | 知识服务。 |
| [`llm/`](llm/_TREE.md) | harness 内的 LLM 服务封装。 |
| [`mcp/`](mcp/_TREE.md) | MCP 接入服务。 |
| [`memory/`](memory/_TREE.md) | 记忆服务与图记忆。 |
| [`orchestration/`](orchestration/_TREE.md) | 智能体编排与规划。 |
| [`pet/`](pet/_TREE.md) | AI 宠物运行时（人格/动画/视觉/语音等）。 |
| [`runtime/`](runtime/_TREE.md) | 依赖容器、消息总线与任务服务。 |
| [`skills/`](skills/_TREE.md) | 技能注册表与规格。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `README.md` | harness 框架说明文档。 |
| `__init__.py` | 包初始化。 |
| `contracts.py` | 框架核心数据契约（技能/轨迹/守护结果等 dataclass）。 |
| `layers.py` | harness 分层定义与元信息。 |
| `ports.py` | 各层的 Protocol 端口接口定义。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
