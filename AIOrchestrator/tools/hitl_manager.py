import asyncio
import logging
from typing import Dict, Optional

logger = logging.getLogger(__name__)


class HITLManager:
    """
    Human-in-the-Loop 确认管理器。
    AIServer 通过 gRPC Confirm RPC 回调授权/拒绝操作。
    """

    def __init__(self):
        self._pending: Dict[str, dict] = {}
        self._results: Dict[str, bool] = {}
        self._lock = asyncio.Lock()

    def submit(self, confirm_id: str, content: str, uid: int) -> None:
        """提交一个待确认的操作"""
        self._pending[confirm_id] = {
            "content": content,
            "uid": uid,
        }
        self._results[confirm_id] = False
        logger.info(f"HITL submitted: confirm_id={confirm_id}, uid={uid}")

    def resolve(self, confirm_id: str, approved: bool) -> None:
        """AIServer 回调授权/拒绝"""
        if confirm_id in self._pending:
            self._results[confirm_id] = approved
            del self._pending[confirm_id]
            logger.info(f"HITL resolved: confirm_id={confirm_id}, approved={approved}")

    def get_pending(self, confirm_id: str) -> Optional[dict]:
        return self._pending.get(confirm_id)

    def is_approved(self, confirm_id: str) -> Optional[bool]:
        return self._results.get(confirm_id)
