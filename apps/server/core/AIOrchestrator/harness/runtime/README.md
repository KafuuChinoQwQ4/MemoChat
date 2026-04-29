# Runtime Layer

Purpose:
- Compose concrete harness services.
- Own singleton lifecycle and startup/shutdown hooks.
- Keep construction details out of orchestration and API routes.

Primary files:
- `container.py`

Coupling rule:
- New concrete services should be wired here and exposed behind ports or small service methods.
