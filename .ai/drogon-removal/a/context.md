# Context

Task date: 2026-05-02

User request: remove Drogon references after noting the project no longer uses Drogon.

Scope:
- Remove framework dependency entries from vcpkg manifest.
- Remove old GateServerDrogon image/build artifacts from deploy config.
- Remove old GateServerDrogon selection/check logic from scripts.
- Update GitHub README and architecture docs to use Boost.Asio/Beast/gRPC wording.
- Replace public docs' old repository/root name wording with MemoChat or <repo-root> where appropriate.

Important distinction:
- Remaining literal `MemoChat-Qml-Drogon` occurrences are local absolute workspace paths in scripts/configs. Those are not the Drogon framework/target and were not renamed to avoid breaking local paths.
