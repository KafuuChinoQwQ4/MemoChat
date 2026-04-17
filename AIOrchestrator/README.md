# AIOrchestrator 微服务

基于 FastAPI + LangChain + LangGraph 的 AI 编排服务，为 MemoChat 提供 AI Agent 能力。

## 技术栈

- **FastAPI + Uvicorn**：异步 HTTP/WebSocket 服务框架
- **LangChain + LangGraph**：Agent 编排框架（ReAct 推理、多工具调用）
- **LLM**：Ollama（本地）/ OpenAI / Anthropic Claude / Moonshot Kimi
- **RAG**：Qdrant 向量数据库 + sentence-transformers 嵌入
- **Neo4j**（未来）：图数据库支持 GraphRAG 和推荐系统
- **三层记忆**：短期（ConversationBufferMemory）、情景（Mem0）、语义（PostgreSQL）

## 快速启动

### 1. 安装依赖

```bash
cd AIOrchestrator
pip install -r requirements.txt
```

### 2. 配置

编辑 `config.yaml`：

```yaml
ollama:
  base_url: http://127.0.0.1:11434
  enabled: true

qdrant:
  host: 127.0.0.1
  port: 6333
  enabled: true
```

### 3. 启动服务

```bash
python main.py
# 或
uvicorn main:app --host 0.0.0.0 --port 8096 --reload
```

## API 端点

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/chat` | AI 对话（非流式） |
| POST | `/chat` | AI 对话（流式，SSE）|
| POST | `/smart` | 智能功能（摘要/翻译/建议） |
| POST | `/kb/upload` | 上传文档到知识库 |
| POST | `/kb/search` | 知识库检索 |
| GET | `/health` | 健康检查 |

## 架构

```
AIServer (C++)          AIOrchestrator (Python)
┌──────────────┐        ┌──────────────────────────────┐
│ gRPC Service │────HTTP────▶│ FastAPI                     │
│ AIServiceImpl│        │  /chat  /smart  /kb/*      │
└──────────────┘        └──────────┬──────────────────┘
                                  │
               ┌──────────────────┼──────────────────┐
               │                  │                  │
          LangGraph          LLM Adapter        RAG Manager
         (ReAct Agent)     (Ollama/OpenAI/     (Qdrant +
          + HITL           Claude/Kimi)      Embedding)
          + Timeout
```

## 工具生态

- 🔍 **搜索工具**：DuckDuckGo 实时搜索
- 🔢 **计算器**：安全数学表达式计算
- 📚 **知识库**：Qdrant 检索增强
- ⏱️ **HITL**：高风险操作需用户确认
- 🔄 **Fallback**：Ollama → OpenAI → Kimi 自动降级
