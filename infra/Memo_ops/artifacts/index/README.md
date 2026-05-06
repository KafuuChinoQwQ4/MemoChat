# Memo_ops Artifacts Index

`Memo_ops/artifacts` is the unified root for local MemoChat operational artifacts.

## Directory Layout

```text
Memo_ops/artifacts/
  logs/
    services/
      GateServer/
      StatusServer/
      chatserver1/
      chatserver2/
      chatserver3/
      chatserver4/
      VarifyServer/
    manual-start/
  loadtest/
    k6/
    runtime/
      accounts/
      logs/
      reports/
    history/
      ad-hoc/
        logs/
        reports/
      e2e/
      suite/
  reports/
    perf/
  index/
    README.md
```

## Classification

- `logs/services`: current and historical service runtime logs
- `logs/manual-start`: console captures and manual-start helper logs
- `loadtest/runtime`: current script outputs and runtime account material
- `loadtest/k6`: k6 scripts and wrappers for HTTP baselines
- `loadtest/history`: historical loadtest samples and archived runs
- `reports/perf`: formal Markdown performance reports intended to remain readable and trackable

## Old Path To New Path

| Old Path | New Path |
|---|---|
| `Memo_ops/runtime/services/**/logs/` | `Memo_ops/artifacts/logs/services/<service>/` |
| `Memo_ops/runtime/varify/logs/` | `Memo_ops/artifacts/logs/services/VarifyServer/` |
| `logs/manual-start/` | `Memo_ops/artifacts/logs/manual-start/` |
| `Memo_ops/runtime/loadtest/` | `Memo_ops/artifacts/loadtest/runtime/` |
| `tools/loadtest/k6/` | `Memo_ops/artifacts/loadtest/k6/` |
| `local-loadtest/logs/` | `Memo_ops/artifacts/loadtest/history/ad-hoc/logs/` |
| `local-loadtest/reports/e2e_*` | `Memo_ops/artifacts/loadtest/history/e2e/<same-dir>/` |
| `local-loadtest/reports/suite_*` | `Memo_ops/artifacts/loadtest/history/suite/<same-dir>/` |
| `local-loadtest/reports/*.json` | `Memo_ops/artifacts/loadtest/history/ad-hoc/reports/` |
| `reports/*.json` | `Memo_ops/artifacts/loadtest/history/ad-hoc/reports/` |
| `reports/*.md` | `Memo_ops/artifacts/reports/perf/` |
| `docs/perf/*.md` | `Memo_ops/artifacts/reports/perf/` |

## Version-Control Rules

- Raw logs and raw JSON outputs should remain ignored
- Runtime account CSV files should remain ignored
- Example input files, templates, and formal Markdown reports may remain tracked

## Notes

- Migration preserves filenames and content
- The goal is path unification, not content rewriting
- If an old document still references a previous location, treat the old path as historical and the path above as authoritative
