# MemoChat

MemoChat 是一个基于 Qt/QML 与 C++/Python 后端的分布式IM应用。仅仅只是个实验性平台，融合传统IM，结合Agent提供现代化赋能。

## 技术栈与依赖总览

- **桌面客户端**：Qt 6.8.3 / Qt 6、QML、Qt Quick、Qt Quick Controls 2、Qt Core、Qt Gui、Qt Widgets、Qt Network、Qt Sql、Qt Concurrent、Qt WebEngineQuick、JavaScript、C++23；MemoOps 客户端使用 Qt Charts；共享客户端层包含本地文件、媒体上传、窗口形状、聊天/联系人/动态/Agent/桌宠/R18 控制器。
- **桌宠与 Live2D 骨架**：GateServer `/ai/pet/*` 代理、AIOrchestrator pet router/runtime、PetController、PetModel、PetWindow、PetScene、Live2DRenderItem 占位渲染边界、Live2DAsset SDK-free 校验、PetAssetSettings 本地 JSON 草稿存储、模型/动作/表情/隐私/调试/置顶/点击穿透控制。
- **服务端 C++**：C++23 默认、Linux GCC16 预设可启用 C++26、Boost.Asio、Boost.Beast、Boost.Filesystem、Boost.PropertyTree、Boost.UUID、gRPC、Protobuf、hiredis、spdlog、glaze、OpenSSL、libpq、libpqxx、mongo-cxx-driver、bsoncxx、mongocxx、AWS SDK for C++ S3、GoogleTest。
- **核心服务**：GateServer、GateServerCore、GateServerHttp1.1、GateServerHttp2、GateServerHttp3、ChatServer、StatusServer、VarifyServer、AIServer、AIOrchestrator、MemoOps。
- **协议与通信**：HTTP/1.1、HTTP/2、HTTP/3、QUIC、gRPC、Protobuf、TCP、SSE、WebSocket、WebRTC、LiveKit bridge、Qt WebEngine bridge；HTTP/2 通过 nghttp2/libh2o 条件启用，HTTP/3 通过 ngtcp2、nghttp3、OpenSSL 条件启用，Windows/QUIC 路径预留 msquic。
- **数据与对象存储**：PostgreSQL 16、Redis Stack、MongoDB 7、MinIO/S3、Qdrant、Neo4j、Postgres asyncpg/psycopg、Redis semantic cache。
- **消息与事件**：Redpanda/Kafka、cppkafka、RabbitMQ、librabbitmq、amqplib、aio-pika、aiokafka、Redis 事件/缓存/锁。
- **AI/RAG 编排**：Python、FastAPI、Uvicorn、LangChain、LangGraph、LangSmith、OpenAI-compatible provider registry、Ollama、Qdrant RAG、Neo4j 图记忆、PostgreSQL、Redis semantic cache、RabbitMQ/Redpanda agent queue、MCP bridge。
- **AI/RAG 库**：langchain-core、langchain-community、langchain-openai、langchain-ollama、langchain-qdrant、qdrant-client、sentence-transformers、rank-bm25、jieba、pypdf、python-docx、arxiv、duckduckgo-search、httpx、pydantic、pydantic-settings、PyYAML、structlog、python-multipart、mcp。
- **模型供应商适配**：Ollama、OpenAI、Claude、Kimi、OpenAI-compatible endpoint；配置中已包含 DeepSeek、Qwen/DashScope、GLM、Gemini、Claude、Kimi、本地 OpenAI-compatible 模型端点等供应商入口。
- **实时音视频**：LiveKit、coturn、WebRTC 房间、Qt WebEngine 内嵌 LiveKit 页面、客户端音视频通话桥接。
- **本地基础设施**：Docker、Docker Compose、Envoy Gateway、Redis Stack、Postgres、MongoDB、MinIO、Redpanda、RabbitMQ、Neo4j、Qdrant、AIOrchestrator Docker 服务。
- **可观测性**：Prometheus、Alertmanager、Grafana、Loki、Tempo、OpenTelemetry Collector、OpenTelemetry SDK/OTLP、InfluxDB、cAdvisor、Envoy admin metrics、LangSmith tracing、Prometheus Python client。
- **运维与部署**：MemoOps QML、OpsCollector Python、Envoy local gateway、Docker bind data、Kubernetes manifests、Helm chart、Kustomize overlays、etcd 配置/服务发现、可选 ELK compatibility stack（Elasticsearch、Logstash、Kibana、Filebeat）。
- **构建与测试**：CMake、CMake Presets、Ninja、vcpkg、MSVC/Windows SDK、GCC16、Qt 6.8.3、GoogleTest、ctest、pytest、Python compileall、PowerShell/ Bash runtime scripts。
- **插件与扩展**：R18SourceService、R18PluginHost、R18MockSourcePlugin、跨平台动态库加载（dlopen/LoadLibrary）、可导入内容源包。
- **Live2D Native 渲染**：Live2D Cubism SDK for Native、Live2D Cubism SDK for Web 临时 fallback、OpenGL/RHI render bridge、`MEMOCHAT_ENABLE_LIVE2D_NATIVE`、`MEMOCHAT_LIVE2D_SDK_ROOT`、`MEMOCHAT_PET_ASSET_ROOT`。当前只提交占位渲染和资源校验，不提交 Live2D SDK、授权模型包或大资源。
- **桌宠运行时**：PetControlEvent、PetObservation、桌宠 session/profile/event 持久化、persona、policy、animation mapper、provider-neutral text/TTS/realtime/multimodal adapters、deterministic provider、interrupt/barge-in、lip sync、gaze、motion、expression、action command。
- **实时语音与 TTS**：API-backed TTS、OpenAI Realtime、Gemini Live、LiveKit/WebRTC realtime bridge、GPT-SoVITS post-MVP voice clone、Faster-Whisper、Whisper.cpp、Silero VAD、WebRTC VAD、RNNoise、SpeexDSP、Rubber Band、SoundTouch、SoX。
- **视觉与多模态**：Qt Multimedia camera/mic、MediaPipe Tasks Vision、OpenCV、ONNX Runtime、CUDA、TensorRT、DirectML（仅 Windows 显式检查）、本地 face/pose/gesture/expression 结构化观察、授权后的 sampled cloud vision、多模态 LLM provider。
- **本地模型与推理服务**：vLLM、Ollama/local omni models、OpenAI-compatible local endpoint、local vision inference sidecar、realtime bridge sidecar。桌宠默认 API-first，不强依赖本地大模型。
- **桌宠数据与媒体资产**：Postgres `ai_pet_profile`、`ai_pet_session`、`ai_pet_event`、`ai_pet_asset`、`ai_voice_profile`，MinIO voice/model/audio refs，Qdrant character lore/memory，Neo4j episodic/relationship graph memory，Redis realtime state/VAD cache/session locks。
- **未来可选服务端口**：GPT-SoVITS API `9880`、local vision inference sidecar `8097`、realtime bridge sidecar `8098`。这些端口需要评审后再启用，不能静默改变现有固定端口。
