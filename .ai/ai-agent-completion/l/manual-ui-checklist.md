# Manual UI Checklist

Use this after local services are up and the client can reach Gate `8080` plus AIOrchestrator `8096`.

1. Open the Agent tab and confirm the model chip shows `qwen3:4b` after model refresh.
2. Open the model dialog with AI services healthy:
   - list appears
   - refresh button disables while loading
   - empty-state text only appears when the list is actually empty
3. Open the KnowledgeBase dialog with no active request:
   - existing KB items render
   - empty-state card shows only when list is empty
4. Trigger KB search:
   - button disables during search
   - status text updates while loading
   - result panel scrolls when the response is long
   - empty-search case shows status text instead of stale old content
5. Trigger KB upload:
   - dialog closes after file selection
   - upload button shows busy state
   - list refresh happens after upload completion, not immediately on click
6. Trigger KB delete:
   - delete button disables during delete
   - item disappears only after backend success
7. Trigger normal AI chat and stream chat:
   - loading / streaming badges update correctly
   - trace dialog shows current trace data after final response
   - request error banner renders actual text, not a blank pill
