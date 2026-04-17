import logging
from typing import Any
from langchain_core.tools import tool

logger = logging.getLogger(__name__)


@tool
def calculator_tool(expression: str) -> str:
    """
    计算器工具。用于执行数学表达式计算。
    请输入有效的 Python 数学表达式。
    """
    try:
        import ast, operator as op
        operators = {
            ast.Add: op.add, ast.Sub: op.sub, ast.Mult: op.mul,
            ast.Div: op.truediv, ast.Pow: op.pow,
            ast.USub: op.neg, ast.UAdd: op.pos,
        }

        def eval_expr(node):
            if isinstance(node, ast.Constant):
                return node.value
            elif isinstance(node, ast.BinOp):
                return operators[type(node.op)](eval_expr(node.left), eval_expr(node.right))
            elif isinstance(node, ast.UnaryOp):
                return operators[type(node.op)](eval_expr(node.operand))
            else:
                raise ValueError(f"不支持的操作: {ast.dump(node)}")

        tree = ast.parse(expression.strip(), mode="eval")
        result = eval_expr(tree.body)
        return f"计算结果: {result}"
    except Exception as e:
        return f"计算错误: {str(e)}"


@tool
def search_tool(query: str) -> str:
    """
    搜索工具。用于获取最新信息、实时数据或网络搜索结果。
    query: 搜索关键词或问题。
    """
    try:
        from duckduckgo_search import DDGS
        with DDGS() as ddgs:
            results = list(ddgs.text(query, max_results=5))
        if not results:
            return "未找到相关结果。"

        lines = []
        for r in results:
            lines.append(f"[{r.get('title', '')}]({r.get('href', '')}): {r.get('body', '')}")
        return "\n".join(lines)
    except Exception as e:
        logger.warning(f"Search failed: {e}")
        return f"搜索暂时不可用: {str(e)}"


@tool
def knowledge_base_tool(query: str, top_k: int = 3) -> str:
    """
    知识库检索工具。根据用户问题检索已上传文档的相关片段。
    query: 用户问题
    top_k: 返回的最相关片段数量
    """
    from rag.knowledge_base import KnowledgeBaseManager
    try:
        kb = KnowledgeBaseManager()
        chunks = kb.search_sync(uid=0, query=query, top_k=top_k)
        if not chunks:
            return "未在知识库中找到相关内容。"

        lines = []
        for i, c in enumerate(chunks, 1):
            lines.append(f"【片段 {i}】（来源: {c.get('source', 'N/A')}）\n{c.get('content', '')}")
        return "\n\n".join(lines)
    except Exception as e:
        logger.warning(f"Knowledge base search failed: {e}")
        return f"知识库检索暂时不可用: {str(e)}"
