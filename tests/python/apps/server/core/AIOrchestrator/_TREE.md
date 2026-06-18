# AIOrchestrator/ 目录树

> 测试 AIOrchestrator（Python AI 编排 harness）服务，覆盖 A2A 游戏、LLM 提供商、RAG、桌宠运行时/路由/特性开关等。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`harness/`](harness/_TREE.md) | harness 子模块（桌宠特性开关等）契约测试 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 将目录标记为测试包 |
| `conftest.py` | pytest 夹具与路径/导入配置 |
| `test_a2a_game_service.py` | 验证 A2A 游戏服务的对局逻辑 |
| `test_harness_structure.py` | 校验 harness 模块结构、LangGraph 编排和运行 trace 契约 |
| `test_langsmith_observability.py` | 验证 LangSmith 可观测性接入 |
| `test_llm_provider_dedupe.py` | 验证 LLM 提供商去重、Kimi 选择和上下文窗口 token 预算保护 |
| `test_ollama_recovery.py` | 验证 Ollama 故障恢复行为 |
| `test_pet_contracts.py` | 校验桌宠相关契约 |
| `test_pet_feature_flags.py` | 验证桌宠特性开关行为 |
| `test_pet_router.py` | 验证桌宠请求路由 |
| `test_pet_runtime.py` | 验证桌宠运行时主流程 |
| `test_pet_runtime_components.py` | 验证桌宠运行时各组件 |
| `test_pet_visual_layer_user_flow.py` | 验证桌宠视觉层的用户流程 |
| `test_rag_retrieval.py` | 验证 RAG 检索流程 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
