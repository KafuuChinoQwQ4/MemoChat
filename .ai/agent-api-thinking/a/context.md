Task: add thinking display, a Think option when supported by the selected model, and API calling for non-local providers such as DeepSeek, Qwen, GLM, Kimi, GPT, Gemini, and Claude.

Relevant files:
- apps/client/desktop/MemoChat-qml/AgentController.h/.cpp
- apps/client/desktop/MemoChat-qml/AgentMessageModel.h/.cpp
- apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml
- apps/client/desktop/MemoChat-qml/qml/agent/AgentMessageDelegate.qml
- apps/server/core/AIOrchestrator/config.py
- apps/server/core/AIOrchestrator/config.yaml
- apps/server/core/AIOrchestrator/schemas/api.py
- apps/server/core/AIOrchestrator/harness/llm/service.py
- apps/server/core/AIOrchestrator/llm/base.py
- apps/server/core/AIOrchestrator/llm/ollama_llm.py
- apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py
- apps/server/core/AIServer/AIServiceJsonMapper.cpp
- apps/server/core/AIServer/AIServiceCore.cpp
- apps/server/core/GateServer/AIServiceClient.cpp
- apps/server/core/common/proto/ai_message.proto

Findings:
- Ollama adapters currently force think=false and strip <think> blocks.
- The harness registry already supports custom OpenAI-compatible endpoints under harness.providers.endpoints, but config only has a local-vllm example.
- OpenAI/Kimi/Claude adapters already exist. DeepSeek/Qwen/GLM/Gemini can be handled as OpenAI-compatible endpoints.
- ModelInfo currently lacks a supports_thinking flag, so the client cannot know when to show Think.

Web source checks:
- Gemini official OpenAI-compatible base URL is https://generativelanguage.googleapis.com/v1beta/openai/ and supports streaming and thinking parameters.
- DeepSeek official API uses Bearer auth and OpenAI-compatible base URL examples at https://api.deepseek.com.
- Alibaba Cloud Model Studio Qwen OpenAI-compatible endpoint includes https://dashscope.aliyuncs.com/compatible-mode/v1.
- Zhipu/GLM official OpenAI-compatible base URL is https://open.bigmodel.cn/api/paas/v4.
- Kimi official docs state OpenAI-compatible base URL https://api.moonshot.ai/v1.

Verification target:
- python -m compileall apps/server/core/AIOrchestrator
- cmake --build --preset msvc2022-full
