# Ollama Stream Timeout

AIOrchestrator tolerates slow CPU-only Ollama first-token latency and reports timeout causes clearly. Local Ollama remains on stable port 11434; AIOrchestrator keeps using Docker service name `memochat-ollama` inside the container network.
