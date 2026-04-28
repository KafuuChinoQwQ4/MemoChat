# Plan

Assessed: yes

Approach:
1. Add source config templates for `StatusServer2` and `VarifyServer2`.
2. Point `GateServer2` at the second Status/Verify pair while keeping `GateServer1` on the first pair.
3. Update deploy script to materialize `StatusServer1/2` and `VarifyServer1/2` runtime directories.
4. Update start/stop scripts to launch and stop both Verify Node processes and both StatusServer exe processes.
5. Deploy from existing build artifacts and run runtime/static verification.

Status:
- [x] Gather context
- [x] Assess port plan
- [x] Patch configs
- [x] Patch scripts
- [x] Deploy and runtime check
- [x] Static verification
- [x] Review
