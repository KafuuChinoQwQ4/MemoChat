# GateServerHttp1.1

`GateServerHttp1.1` is a legacy opt-in standalone executable. It is not the production or default GateServer entrypoint.

The default backend entrypoint is `GateServer`, which owns the production HTTP/1.1 surface and transport integration path.

This directory remains in the build graph only for compatibility with the legacy standalone target. R18 helper targets `R18PluginHost` and `R18MockSourcePlugin` live under `GateServer/plugins/r18` and remain buildable by default.

To build the legacy standalone executable explicitly, configure CMake with:

```bash
cmake --preset linux-full-gcc16 -DMEMOCHAT_BUILD_LEGACY_GATE_HTTP1_STANDALONE=ON
```
