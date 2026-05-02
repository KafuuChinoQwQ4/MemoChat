# Verification

Network/source checks:

- `Invoke-WebRequest -UseBasicParsing -Uri "https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/index.json"`: exit 0. Returned Venera's official source catalog; entries include `name`, `fileName`, `key`, `version`, and optional `description`.
- `Invoke-WebRequest -UseBasicParsing -Uri "https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/nhentai.js"`: exit 0. Returned a JavaScript source script, proving relative script URLs from the catalog are reachable.

Static checks:

- `git diff --check -- apps/client/desktop/MemoChat-qml/R18Controller.h apps/client/desktop/MemoChat-qml/R18Controller.cpp apps/client/desktop/MemoChat-qml/R18ListModel.cpp apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml apps/server/core/GateServerHttp1.1/H1R18Routes.cpp docs/当前架构基准.md docs/API参考.md docs/架构文档.md .ai/r18-venera-ui/b`: exit 0. Git reported LF-to-CRLF working-copy warnings only.
- `qmllint apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`: exit 0. It reported existing Qt6 delegate unqualified-access warnings, not syntax errors.

Builds:

- `cmake --preset msvc2022-client-verify`: exit 0. Optional `WrapVulkanHeaders` warning only.
- `cmake --build --preset msvc2022-client-verify`: initially failed because `R18Controller.cpp` had lambda capture mistakes around `url`; fixed and reran. Final exit 0, linked `bin\Release\MemoChatQml.exe`.
- `cmake --preset msvc2022-server-verify`: exit 0. Existing CMake dev/deprecation warnings for Boost/utf8proc.
- `cmake --build --preset msvc2022-server-verify`: exit 0, rebuilt and linked `GateServerHttp1.1.exe`.

Runtime boundary:

- This turn verifies catalog/script fetching and staging support. It does not verify real comic search/detail/page loading from Venera JS scripts because that requires the future Venera JS API runtime adapter.

