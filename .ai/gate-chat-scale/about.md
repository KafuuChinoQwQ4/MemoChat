# Gate Chat Scale

MemoChat local runtime supports two GateServer instances and six ChatServer instances.

GateServer1 keeps the default client entrypoint on HTTP `8080` and HTTP/2 `8082`. GateServer2 uses HTTP `8084` and HTTP/2 `8085`. ChatServer instances are `chatserver1` through `chatserver6`; their TCP ports are `8090`, `8091`, `8092`, `8093`, `8094`, and `8097`, with RPC ports `50055` through `50059` plus `50381` for `chatserver6`.
