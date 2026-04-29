# Feedback Layer

Purpose:
- Persist run traces and step traces.
- Read traces back after process restart.
- Store user feedback and produce automatic feedback summaries.

Primary files:
- `trace_store.py`: trace lifecycle and Postgres persistence.
- `evaluator.py`: lightweight automatic confidence/summary signal.

Coupling rule:
- Do not call orchestration from this layer. Feedback receives trace events and observations as data.
