Review result: passed.

Checked:
- The generation status badge still floats at the top right, but the message list now shifts down while loading/streaming so it does not cover chat bubbles.
- Thinking content is no longer always exposed above the final answer after completion.
- Delegate-local state resets on `msgId` changes, which avoids recycled ListView delegates carrying an expanded state into another message.

Residual risk:
- This was build-verified but not screenshot-verified in a running desktop window.
