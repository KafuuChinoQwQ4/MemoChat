# Plan

Assessed: yes

Approach:
1. Update startup, stop, and deploy scripts so local runtime owns `GateServer1`, `GateServer2`, and `chatserver1` through `chatserver6`.
2. Update source config templates so GateServer, StatusServer, and ChatServer all see the same six-node static cluster.
3. Add per-instance ChatServer source configs for `chatserver5` and `chatserver6`.
4. Update Docker local LB metadata for two gate instances and six chat instances without changing datastore ports.
5. Materialize current runtime directories/configs for the new topology.
6. Run static checks only.

Status:
- [x] Gather context
- [x] Assess port plan
- [x] Patch scripts
- [x] Patch source configs
- [x] Patch runtime topology
- [x] Static verification
- [x] Review
