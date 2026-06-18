# rag/ 目录树

> 检索增强生成（RAG）子系统：文档处理、嵌入、检索与生成链。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `__init__.py` | 包初始化。 |
| `chain.py` | RAG 链：文档上传→向量存储→检索→注入 LLM。 |
| `doc_processor.py` | 文档加载与分块（PDF/TXT/MD/DOCX）。 |
| `embedder.py` | 嵌入管理器：sentence-transformers/Ollama/OpenAI 等。 |
| `retrieval.py` | 检索实现（含混合/重排逻辑）。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
