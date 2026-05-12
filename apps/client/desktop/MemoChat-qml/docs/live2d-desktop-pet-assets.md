# Live2D Desktop Pet SDK And Asset Policy

This project keeps Live2D support opt-in. The default MemoChat client builds with
the placeholder `Live2DRenderItem` and does not require the Live2D Cubism SDK,
model packages, generated audio, voice references, or provider credentials.

## Local Paths

- Put the Live2D Cubism SDK for Native outside the repository, preferably under
  `/data/third_party/live2d/CubismSdkForNative-current` on Arch/WSL.
- Put user-supplied pet model packages under `/data/memochat/pet-assets` or a
  user-selected local directory.
- Put large caches, generated audio, model caches, and experiments under `/data`.
- Do not use `D:` except for legacy Windows checks, backups, or explicit
  Windows-side validation.

## CMake Flags

The native renderer path is disabled by default:

```bash
cmake --preset linux-client-gcc16
```

To prepare a local native-renderer build, provide an SDK root explicitly:

```bash
cmake --preset linux-client-gcc16 \
  -DMEMOCHAT_ENABLE_LIVE2D_NATIVE=ON \
  -DMEMOCHAT_LIVE2D_SDK_ROOT=/data/third_party/live2d/CubismSdkForNative-current
```

When `MEMOCHAT_ENABLE_LIVE2D_NATIVE=OFF`, CMake does not require the SDK and the
placeholder renderer stays active. When the flag is `ON`, an empty or invalid
SDK root should fail during configure with a clear message.

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
will be added behind `MEMOCHAT_ENABLE_LIVE2D_NATIVE` and the existing
`Live2DRenderItem` visual-state contract. Missing SDKs, missing models, and
invalid packages should produce recoverable UI diagnostics rather than blank or
crashing windows.
