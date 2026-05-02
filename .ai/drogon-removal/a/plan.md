# Plan

Summary: remove real Drogon framework/target references while preserving working local paths.

Checklist:
- [x] Remove `drogon` and `trantor` from vcpkg.json.
- [x] Delete old GateServerDrogon Dockerfiles.
- [x] Remove GateServerDrogon from binary selection/minimal test script.
- [x] Update README and architecture docs away from Drogon wording.
- [x] Run search for framework/target references, filtering the current workspace path name.
- [x] Run diff check.
- [x] Run server configure/build verification.

Assessed: yes
