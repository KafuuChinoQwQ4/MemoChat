# AI Agent Transparency

MemoChat AI Agent exposes a transparent execution trail for each run. The trace shows request invocation, skill planning, orchestration steps, memory/context retrieval, tool calls, knowledge/web/search observations, model execution, memory write-back, feedback evaluation, and the node-to-node flow projected from persisted trace events.

The UI renders the trace as a readable timeline, with compact cards for each layer and expandable metadata summaries that stay safe for users by avoiding secrets and hidden chain-of-thought.
