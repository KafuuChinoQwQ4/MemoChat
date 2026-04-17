"""
计算器工具 — 安全数学计算
"""
import math
import structlog
import re
from langchain_core.tools import tool

logger = structlog.get_logger()


class CalculatorTool:
    """计算器工具 — 安全数学计算，避免 eval"""

    @tool("calculator", description="执行数学计算。输入数学表达式（如 2+3*4），返回计算结果。支持加减乘除、幂运算、平方根等。")
    async def calculator(self, expression: str) -> str:
        """
        执行安全数学计算。
        只允许数字、运算符和数学函数。
        """
        allowed_names = {
            "abs": abs, "round": round, "min": min, "max": max,
            "pow": pow, "sqrt": math.sqrt, "sin": math.sin,
            "cos": math.cos, "tan": math.tan, "log": math.log,
            "log10": math.log10, "exp": math.exp, "pi": math.pi, "e": math.e,
            "floor": math.floor, "ceil": math.ceil,
        }

        expression = expression.strip()

        if not re.match(r'^[\d\s\+\-\*\/\.\(\)\,\%\^\+\-]+$', expression):
            return f"非法表达式: {expression}"

        try:
            import ast
            import operator as op

            def eval_expr(node):
                if isinstance(node, ast.Num):
                    return node.n
                elif isinstance(node, ast.BinOp):
                    return ops[type(node.op)](eval_expr(node.left), eval_expr(node.right))
                elif isinstance(node, ast.UnaryOp):
                    return ops[type(node.op)](eval_expr(node.operand))
                elif isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
                    return allowed_names[node.func.id](*[eval_expr(arg) for arg in node.args])
                else:
                    raise ValueError(f"Unsupported syntax: {ast.dump(node)}")

            ops = {
                ast.Add: op.add, ast.Sub: op.sub,
                ast.Mult: op.mul, ast.Div: op.truediv,
                ast.Pow: op.pow, ast.Mod: op.mod,
            }

            tree = ast.parse(expression, mode="eval")
            result = eval_expr(tree.body)
            return str(result)

        except ZeroDivisionError:
            return "错误: 除数不能为零"
        except Exception as e:
            logger.warning("calculator.error", expression=expression, error=str(e))
            return f"计算错误: {str(e)}"

    def get_tool(self):
        return self.calculator
