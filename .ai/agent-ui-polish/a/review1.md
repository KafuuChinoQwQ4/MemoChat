# Review 1

Findings:
- No blocking issues found in the scoped client/QML diff.

Notes:
- The original `error=2` path had two causes: the client used POST for GET-only Gate routes, and the local runtime did not have AIServer available when Gate handled AI routes.
- `start-all-services.bat` now launches AIServer before Gate, and `stop-all-services.bat` stops/checks AIServer as part of cleanup.
- The QML panel bounds now use parent-limited dimensions, and the panel roots are clipped.
- AI streaming content now assigns the accumulated content instead of appending it twice.

Residual risk:
- QML visual polish was not screenshot-verified in this pass.
