Review result: passed.

Checked:
- `cancelStream()` no longer dereferences `_currentStreamReply` after `abort()`.
- Stream signals are disconnected before aborting a user-cancelled reply, so `onStreamFinished()` cannot race the explicit cancel cleanup.
- The assistant message model is finalized during cancellation, preventing a stale streaming bubble.

Residual risk:
- This was build-verified, not reproduced under an attached debugger.
