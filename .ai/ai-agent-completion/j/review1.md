# Review 1

Findings:

- Fixed during implementation: the first version of the AIServer mapping helper reused `GlazeCompat` accessors on `const JsonValue` temporaries, which produced empty model/KB arrays under test. Replaced that with explicit object/array traversal against the underlying Glaze generic JSON container.
- Fixed during verification: helper overloads initially assumed `default_model` would arrive as `JsonMemberRef`; updated the mapper to handle both array items and standalone object values cleanly.

Residual risk:

- The Ollama recovery path is intentionally conservative: one retry after client reset plus readiness probing. If startup windows become longer than the current probe loop, requests can still fail, but the failure mode is now logged clearly and should self-heal on the next successful readiness window.
- The config/runtime consistency issue for AIOrchestrator model inventory after container rebuilds is still a separate follow-up item.

Conclusion:

- This phase closes the two targeted maintenance gaps:
  - AIServer model list and KB list/delete mappings now have isolated regression coverage.
  - AIOrchestrator now resets and re-probes Ollama after transient `/api/chat` failures instead of surfacing a bare one-shot 404 immediately.
