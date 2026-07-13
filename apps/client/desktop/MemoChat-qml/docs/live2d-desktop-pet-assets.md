# Live2D Desktop Pet SDK And Asset Policy

This project keeps Live2D support opt-in. The default MemoChat client can build
without the Live2D Cubism SDK, model packages, generated audio, voice
references, or provider credentials. If the native renderer is unavailable or a
model cannot be displayed, `Live2DRenderItem` shows an explicit model error diagnostic
instead of a placeholder character.

## Local Paths

- Put the Live2D Cubism SDK for Native outside the repository, preferably under
  `/data/third_party/live2d/CubismSdkForNative-current` on Arch/WSL.
- Put user-supplied pet model packages under `/data/memochat/pet-assets` or a
  user-selected local directory.
- Developer model packages kept under `resources/live2d` are local-only and
  ignored by Git; clean clones do not include a default model or voice.
- Put large caches, generated audio, model caches, and experiments under `/data`.
- Do not use `D:` except for legacy Windows checks, backups, or explicit
  Windows-side validation.

## CMake Flags

MemoChat keeps a single Linux preset for normal development. It builds the full
stack and enables the native Live2D renderer from the local SDK path:

```bash
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

Windows full builds do not require the Linux SDK path:

```powershell
cmake --preset msvc2022-full
cmake --build --preset msvc2022-full
```

When `MEMOCHAT_ENABLE_LIVE2D_NATIVE=OFF`, CMake does not require the SDK and the
runtime item reports a model display error. The checked-in Linux full preset sets
`MEMOCHAT_ENABLE_LIVE2D_NATIVE=ON`; an empty or invalid SDK root should fail
during configure with a clear message.

## What Must Not Be Committed

The following items must not be committed:

- Live2D Cubism SDK binaries, archives, and redistributable libraries.
- Licensed `.model3.json`, `.moc3`, texture, motion, expression, physics, pose,
  user-data, or VTube Studio mapping files unless the license is explicitly
  repo-safe.
- Generated speech clips, reference voice audio, trained voice artifacts, or
  local model weights.
- API keys, provider secrets, sampled camera frames, or raw microphone captures.

Repo-owned test fixtures may remain under `tests/fixtures/live2d` only when they
are minimal synthetic assets created for validation tests.

## Runtime Boundary

`Live2DAsset` validates local packages without linking the SDK. Native rendering
only consumes the model selected through the existing `Live2DRenderItem`
contract; an empty selection stays empty. Missing SDKs, missing models, and
invalid packages produce recoverable UI diagnostics rather than crashing the
client.
