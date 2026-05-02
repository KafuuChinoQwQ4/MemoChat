# AI Markdown Render

MemoChat desktop renders AI assistant Markdown at the UI/model boundary while keeping the original assistant message text unchanged. The `AgentMessageModel` exposes both raw content and sanitized rich text, and the QML assistant bubble uses the rich text role only for assistant replies without error state.

The renderer is intentionally dependency-light for the first pass. It supports common LLM output shapes such as headings, bullets, numbered lists, blockquotes, fenced code blocks, inline code, bold, italic, and sanitized links.
