# Context

## User Report

用户在 AI 界面发现三个问题：

1. 刚进入 AI 界面时，首次点击某个会话没有加载出消息；切到朋友圈等其他界面再切回 AI 界面后才显示。
2. 第一次输入并得到回答后，再想继续输入时，输入框没有插入光标，也无法输入；切到其他界面再切回后恢复。
3. AI Agent 输入框提示文字偏白，和背景相近，看不清。

## Relevant Files

- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
  - `switchSession()`
  - `loadHistory()`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentComposerBar.qml`
  - AI Agent 输入框 `TextEdit`
  - placeholder `Text`
  - `enabledComposer` busy gating
- `apps/client/desktop/MemoChat-qml/qml/ChatShellPage.qml`
  - `agentPaneLoader` lazy-loads `AgentPane`
- `apps/client/desktop/MemoChat-qml/qml/chat/ChatLeftPanel.qml`
  - left-side AI session list emits `agentSessionSelected`

## Diagnosis

The first issue can happen when the current session is already selected by controller state before the user clicks it. The previous `switchSession()` returned immediately for the same `sessionId`, so it did not send a history request. Re-entering the AI tab later caused initialization/rebinding to reveal already loaded state or trigger a separate refresh.

The second issue is a QML focus lifecycle problem. `AgentComposerBar` uses a plain `TextEdit` inside a `ScrollView`; it did not explicitly regain focus after the `enabledComposer` state flipped back from busy/streaming to ready. Switching tabs destroys/recreates or reactivates the loader, which incidentally gives focus back.

The third issue is visual contrast: placeholder color used a semi-transparent light blue-gray on a pale glass background.

## Scope

This fix is limited to AI client UI behavior. It does not change the AI HTTP contract or server behavior.
