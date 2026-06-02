from __future__ import annotations

import sys

from tests.python.support.paths import ai_orchestrator_root

AI_ORCHESTRATOR_ROOT = str(ai_orchestrator_root())
if AI_ORCHESTRATOR_ROOT not in sys.path:
    sys.path.insert(0, AI_ORCHESTRATOR_ROOT)
