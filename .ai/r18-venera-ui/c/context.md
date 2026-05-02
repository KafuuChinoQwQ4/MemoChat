# Context

Task: user clarified that MemoChat should not require loading Venera itself or Venera's catalog as the main flow. Like Venera, it should directly pull comic source scripts such as `jm.js`.

Start time: 2026-05-02T00:33:38.2833456-07:00.

Existing state from `.ai/r18-venera-ui/b`:

- Client can load Venera catalog and import selected JS source scripts.
- Server can accept JavaScript source payloads and stage them as `venera-js`.
- Search/detail/page execution from JS scripts is still not implemented; staged scripts do not yet drive real results.

New correction:

- Make direct source URL import the primary UI path.
- Default the direct URL field to JM source: `https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/jm.js`.
- Keep catalog loading as a helper/optional discovery flow, not the primary requirement.

Evidence:

- `Invoke-WebRequest -UseBasicParsing -Uri "https://cdn.jsdelivr.net/gh/venera-app/venera-configs@main/jm.js"` returned the JM JavaScript source script.
- The user also has a local Venera source copy open at `C:\Users\yangkeshilie\AppData\Roaming\com.github.wgh136\venera\comic_source\jm.js`, confirming this is the type of source to pull.

Relevant files:

- `apps/client/desktop/MemoChat-qml/R18Controller.{h,cpp}`: add direct source URL import method.
- `apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`: make direct JS URL import prominent and default to JM.
- Docs should say direct JS source URLs such as `jm.js` are the primary import path; catalog is optional discovery.

Verification:

- `qmllint apps/client/desktop/MemoChat-qml/qml/r18/R18ShellPane.qml`
- `cmake --build --preset msvc2022-client-verify`

