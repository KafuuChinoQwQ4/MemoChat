# Runtime Update Restart Plan

Assessed: yes

Checklist:
- [x] Check service scripts, processes, and Docker state.
- [x] Sync verified build outputs into deploy source.
- [x] Deploy runtime services.
- [x] Start updated AIServer and GateServer.
- [x] Probe health/API behavior and record result.

Additional runtime hardening completed:
- [x] Fixed Gate POST empty replies caused by stale connection deadlines.
- [x] Fixed Singleton overload double-construction risk.
- [x] Fixed safe JSON reads for missing numeric/bool values.
- [x] Fixed Gate AI nested JSON response construction.
- [x] Fixed AIServer session repo concurrent pqxx transaction access.
- [x] Added AI stream timeout/error propagation so failures return structured SSE instead of empty body where possible.
