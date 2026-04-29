Verification:
- git diff --check: no whitespace errors; only existing CRLF conversion warnings were printed.
- cmake --build --preset msvc2022-full: first long run timed out at 124 seconds before completion.
- cmake --build --preset msvc2022-full: second run reached link but failed with LNK1104 for GateServerHttp1.1.exe and VarifyServer.exe, consistent with transient output executable locking.
- Process/file checks found no matching running GateServerHttp1.1.exe or VarifyServer.exe and files were not read-only.
- cmake --build --preset msvc2022-full: retry succeeded, linking GateServerHttp3.lib, MemoChatQml.exe, and GateServer.exe.
- After a final QML title fallback tweak, git diff --check again reported no whitespace errors and cmake --build --preset msvc2022-full succeeded, rebuilding qml.qrc and MemoChatQml.exe.
