# Plan

## Summary

Fix AI Agent empty-output behavior, tool compatibility, code-block rendering, and model-list persistence/filtering.

## Steps

- [x] Patch DuckDuckGo async compatibility.
- [x] Robustly strip think blocks and retry/fail empty model output without using safety-block wording.
- [x] Filter Ollama models from actual `/api/tags` and remove unavailable Qwen config entries.
- [x] Persist runtime API providers through Docker volume and cache model lists in the QML client.
- [x] Improve Markdown fenced code block rendering.
- [x] Run targeted Python tests/builds and client verify.

Assessed: yes
