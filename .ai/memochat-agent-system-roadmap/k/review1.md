# Review 1

Findings: none blocking.

Checked:
- `AgentCard`, `AgentCardSkill`, `AgentCapabilities`, and `RemoteAgentRef` are additive contracts.
- `AgentInteropService.local_card()` builds skills from local AgentSpecs and AgentFlows instead of duplicating capability definitions.
- Remote agent registration validates HTTP(S) URLs and defaults to placeholder-only invocation.
- API endpoints are additive and do not alter existing chat, task, eval, spec, or flow contracts.
- Tests cover card shape, remote registration/update/delete, and invalid remote URL rejection.

Residual risk:
- Full A2A JSON-RPC task execution is not implemented. The current layer prepares discovery and registry metadata while keeping remote invocation disabled until trust and transport policies are designed.
