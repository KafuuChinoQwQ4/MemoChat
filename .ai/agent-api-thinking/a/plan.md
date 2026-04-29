Summary: add thinking-mode UI/data plumbing and external API provider configuration.

Approach:
1. Add supports_thinking metadata to provider/model configs and propagate it through /models to the client.
2. Add AgentController thinkingEnabled/currentModelSupportsThinking and include metadata.think in chat payloads.
3. Preserve/extract <think> blocks and provider reasoning_content into AgentMessageModel thinkingContent and render it in AgentMessageDelegate with lighter styling.
4. Extend AIOrchestrator provider config with external API endpoints for DeepSeek, Qwen, GLM, Gemini, Kimi, GPT/OpenAI, Claude and generic OpenAI-compatible routing.
5. Verify Python compile and full CMake build.

Status:
- [x] Context gathered
- [x] Plan assessed against existing symbols
- [x] Implement model/provider metadata
- [x] Implement thinking data/display
- [x] Implement external API provider config/routing
- [x] Verify builds
- [x] Review diff

Assessed: yes
