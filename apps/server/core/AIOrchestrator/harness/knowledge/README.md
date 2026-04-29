# Knowledge Layer

Purpose:
- Upload documents into the RAG pipeline.
- Search and delete user knowledge bases.
- Keep Postgres knowledge-base metadata aligned with Qdrant collections.

Primary files:
- `service.py`

Coupling rule:
- This layer owns knowledge-base workflows. Execution should call it through a service boundary and receive search hits as data.
