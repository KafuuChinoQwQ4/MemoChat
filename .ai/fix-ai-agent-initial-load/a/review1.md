# Review 1

Checked diff for lifecycle and UI risk.

Findings:
- AppController, ChatShellPage, and ChatLeftPanel now converge on AgentController::ensureInitialized() for first AI-page entry. This avoids missing the async Loader creation timing without forcing repeated refreshes on every visible/load event.
- ensureInitialized() is keyed by uid so switching accounts can initialize the AI pane again.
- Manual direct refresh paths remain available through loadSessions()/refreshModelList() if callers need forced refresh later.
- Login glass changes are color/opacity only; no layout, resource, or import changes.

Residual risk:
- Runtime visual validation was not captured via screenshot in this turn; build verification covered C++/QML resource compilation.
