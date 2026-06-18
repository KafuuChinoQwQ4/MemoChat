# llm/ 目录树

> 多家 LLM 提供方的适配层与统一管理器，向编排服务屏蔽具体厂商差异。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 包初始化。 |
| `base.py` | LLM 抽象基类与消息类型定义。 |
| `claude_llm.py` | Anthropic Claude 适配实现。 |
| `kimi_llm.py` | Kimi（Moonshot）适配实现。 |
| `manager.py` | LLM 管理器：按配置选择并调度提供方。 |
| `ollama_llm.py` | 本地 Ollama 适配实现。 |
| `openai_llm.py` | OpenAI 适配实现。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
