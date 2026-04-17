"""
文档处理器 — PDF / TXT / MD / DOCX 加载与分块
"""
import io
from typing import Optional
import structlog
from langchain_community.document_loaders import PyMuPDFLoader, TextLoader, UnstructuredMarkdownLoader
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain_core.documents import Document

from config import settings

logger = structlog.get_logger()


class DocProcessor:
    def __init__(self, rag_config=None):
        self.rag_config = rag_config or settings.rag
        self.chunk_size = self.rag_config.chunk_size
        self.chunk_overlap = self.rag_config.chunk_overlap
        self.splitter = RecursiveCharacterTextSplitter(
            chunk_size=self.chunk_size,
            chunk_overlap=self.chunk_overlap,
            separators=["\n\n", "\n", "。", "！", "？", " ", ""],
            length_function=len,
        )
        self._loader_map = {
            "pdf": PyMuPDFLoader,
            "txt": TextLoader,
            "md": UnstructuredMarkdownLoader,
        }

    async def process(self, file_bytes: bytes, file_type: str, file_name: str) -> list[Document]:
        """
        加载文档 → 分块 → 添加元数据
        """
        try:
            loader_cls = self._loader_map.get(file_type)
            if loader_cls is None:
                raise ValueError(f"Unsupported file type: {file_type}")

            if file_type == "txt":
                loader = loader_cls(file_bytes if isinstance(file_bytes, str) else file_bytes.decode("utf-8", errors="replace"))
            elif file_type == "md":
                loader = loader_cls(file_bytes.decode("utf-8", errors="replace") if isinstance(file_bytes, bytes) else file_bytes)
            else:
                bio = io.BytesIO(file_bytes)
                loader = loader_cls(bio)

            docs = loader.load()

            chunks = self.splitter.split_documents(docs)

            for i, chunk in enumerate(chunks):
                chunk.metadata.update({
                    "source": file_name,
                    "chunk_index": i,
                    "file_type": file_type,
                    "total_chunks": len(chunks),
                })

            logger.info("doc.processed", file_name=file_name, chunks=len(chunks))
            return chunks

        except Exception as e:
            logger.error("doc.process.failed", file_name=file_name, error=str(e))
            return []
