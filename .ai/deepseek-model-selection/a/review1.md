# Review 1

Reviewed `AgentController.cpp` and `AgentController.h` changes.

Findings:
- No blocking issues found in the new selection logic.
- The fix preserves user intent: explicit model clicks are persisted, while automatic/default Ollama selection can be replaced by an available API model.
- On API provider deletion, the explicit flag is cleared when the current model came from an API provider so refresh can choose a valid fallback.

Residual risk:
- Existing users who intentionally use Ollama but never explicitly clicked it may be auto-switched to the first API model when an API provider exists. That matches the DeepSeek registration workflow, but we may later add an explicit default-model selector if both local and API models are commonly used.
