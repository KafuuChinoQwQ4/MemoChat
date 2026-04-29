Task: fix a desktop client crash when the user clicks stop while the AI is streaming a response.

Relevant files:
- apps/client/desktop/MemoChat-qml/AgentController.cpp
- apps/client/desktop/MemoChat-qml/AgentController.h
- apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml

Finding:
- `cancelStream()` called `_currentStreamReply->abort()` and then `_currentStreamReply->deleteLater()`.
- In Qt, `abort()` can synchronously emit `finished`, which enters `onStreamFinished()` and sets `_currentStreamReply` to null. Returning to `cancelStream()` could then dereference a null pointer.
- Cancellation also cleared `_currentStreamMsgId` without finalizing the assistant message, leaving the UI in a streaming state.

Implementation:
- Store `_currentStreamReply` in a local pointer, set the member to null first, disconnect this controller, then abort and deleteLater the local reply.
- Finalize the current assistant message on cancel. Preserve partial content if any; otherwise show "已停止生成".
- Apply the same safe detach/abort/delete pattern in the destructor.

Verification:
- `cmake --build --preset msvc2022-full`
