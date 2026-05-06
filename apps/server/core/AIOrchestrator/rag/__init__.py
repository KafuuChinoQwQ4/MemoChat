"""
RAG 包初始化
"""
from __future__ import annotations

from importlib import import_module

__all__ = ["RAGChain", "EmbeddingManager", "DocProcessor"]


def __getattr__(name: str):
    if name == "RAGChain":
        return import_module(".chain", __name__).RAGChain
    if name == "EmbeddingManager":
        return import_module(".embedder", __name__).EmbeddingManager
    if name == "DocProcessor":
        return import_module(".doc_processor", __name__).DocProcessor
    raise AttributeError(name)

