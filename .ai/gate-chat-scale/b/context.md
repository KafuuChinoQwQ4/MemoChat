# Context

Task:
- Extend the local two-Gate / six-Chat topology with two StatusServer instances and two VarifyServer instances.

Port plan:
- StatusServer1: gRPC `50052`
- StatusServer2: gRPC `50382`
- VarifyServer1: gRPC `50051`, health `8083`
- VarifyServer2: gRPC `50383`, health `8087`

Routing:
- GateServer1 uses StatusServer1 and VarifyServer1.
- GateServer2 uses StatusServer2 and VarifyServer2.

Notes:
- `50060-50159` is reserved by Windows on this machine, so new gRPC ports use `50382` and `50383`.
- VarifyServer runtime startup uses the Node.js implementation in `apps/server/core/VarifyServer/server.js`, configured by environment variables.

