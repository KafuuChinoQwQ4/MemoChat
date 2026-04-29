# AI Agent Output And Session Delete

The AI Agent chat path uses streaming for user questions so long LLM responses are not cut off by the normal HTTP timeout. Streamed chunks update the local assistant placeholder immediately, normal remote-close events after useful SSE content do not become user-visible `error=2`, and deleting the selected session clears the current session and message view.
