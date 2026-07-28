# C++ Service Images

`services/cpp-service.Dockerfile` is a runtime-only image shared by the current
C++ service targets. It never builds from the repository and never copies
`infra/Memo_ops/runtime`, logs, credentials, TLS keys, database files, or source
configuration into an image.

## Build contract

Build the server-only release preset, then create fresh per-service bundles:

```bash
source tools/scripts/release/load_build_environment.sh /root/.memochat-linux-env
cmake --preset linux-server-release-gcc16
cmake --build --preset linux-server-release-gcc16

BUNDLE_ROOT=/data/releases/memochat/backend-bundles-v1
SOURCE_SHA="$(git rev-parse HEAD)"
tools/scripts/release/package_backend_services.sh \
  --library-dir "$VCPKG_ROOT/installed-memochat-gcc16-server-release/x64-linux-memochat-release/lib" \
  --vcpkg-installed-root "$VCPKG_ROOT/installed-memochat-gcc16-server-release" \
  --vcpkg-triplet x64-linux-memochat-release \
  --source-sha "$SOURCE_SHA" \
  --output "${BUNDLE_ROOT}"
tools/scripts/release/verify_release_tree.sh "${BUNDLE_ROOT}"
```

The output root must not already exist. Each named build context has this
allowlisted layout:

```text
<TARGET>/
  bin/<TARGET>
  lib/libmsquic.so.2
  lib/libstdc++.so.6
  lib/libgcc_s.so.1
  lib/libatomic.so.1
  legal/LICENSE
  legal/THIRD_PARTY_NOTICES.md
  legal/LEGAL-STATUS.txt       # inventory/corpus/formal-release state
  legal/third-party/           # present only after the formal corpus verifies
  sbom/vcpkg-build-dependencies.spdx.json
  MANIFEST.txt
  SHA256SUMS
```

The packaging step strips binaries, writes a relative RUNPATH, recursively
copies required compiler/non-system runtime libraries, and rejects unresolved
dependencies. Its initial and recursive `ldd` calls use a clean loader
environment: every non-system dependency must resolve below an explicit,
canonical `--library-dir`; ambient `LD_LIBRARY_PATH` and `LD_PRELOAD` values are
not trusted. The vcpkg SPDX is a fail-closed over-approximation of every
installed base package for the release triplet and is bound to `SOURCE_SHA`.
`verify_release_tree.sh` is still mandatory: the Docker build does not hide
developer paths or suspicious content that remain in an ELF.

Generate the separately scanned, repository-shaped local deployment kit with:

```bash
tools/scripts/release/package_backend_deployment_kit.sh \
  --output /data/releases/memochat/backend-deployment-v1
```

It contains the fixed release Compose files/wrapper, sanitized public INI files,
all business migrations, and the PostgreSQL/MongoDB/MinIO provisioners. Normal
local packaging fails if the root MIT license or third-party inventory is
missing or malformed. It otherwise writes `LEGAL-STATUS.txt` and may explicitly
report an incomplete formal corpus. Versioned tag CI additionally fails closed
until `legal/third-party` passes scope, checksum, approval-state, and release
commit binding checks; only then is that corpus copied into bundles and the
deployment kit.

Build all 15 Compose images after verification:

```bash
tools/scripts/release/build_backend_images.sh \
  --bundle-root "${BUNDLE_ROOT}" \
  --image-prefix memochat \
  --tag local
```

The script owns the explicit target-to-image mapping, verifies every bundle
before the first image mutation, and uses `docker buildx build --load`. It gives
BuildKit only the verified bundle, the server entrypoint, and an empty default
context; private environment or TLS files beside the deployment kit are never
part of the build context. The runtime base is the immutable
`ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90`
reference. Local builds use `--pull=false`, which allows a preloaded digest to
build when Docker Hub is unavailable; pass `--pull` only when the fixed digest
must be fetched from the registry. Apt sources are rewritten to the fixed Ubuntu
`20260727T000000Z` snapshot before package resolution. Because the minimal base
has no CA bundle, the Dockerfile first extracts Mozilla certificate data from
the snapshot's `ca-certificates_20260601~24.04.1_all.deb`, protected by SHA-256
`6bac2a01979e210d9eac1d4d56747ec709ea60654744d66705dc3c36e7629e50`.
The final stage installs and asserts exactly
`ca-certificates=20260601~24.04.1`, `libturbojpeg=1:2.1.5-2ubuntu2`, and
`libwebp7=1.3.2-0.4build3`; image labels preserve the base digest, snapshot,
bootstrap digest, and package tuple for audit.

For a focused diagnostic build, pass one service bundle as a BuildKit named
context directly:

```bash
TARGET=ChatServer
IMAGE=memochat/chat-server:local
EMPTY_CONTEXT="$(mktemp -d)"
trap 'rm -rf -- "$EMPTY_CONTEXT"' EXIT
docker buildx build --load \
  --build-context "service_bundle=${BUNDLE_ROOT}/${TARGET}" \
  --build-context "server_entrypoint=infra/deploy/images/common/entrypoints" \
  --build-arg "TARGET=${TARGET}" \
  --file infra/deploy/images/services/cpp-service.Dockerfile \
  --tag "${IMAGE}" \
  "${EMPTY_CONTEXT}"
```

The Dockerfile validates the target allowlist, manifest, checksums, vcpkg SBOM,
`libmsquic.so.2`, gcc16 runtime libraries, and final `ldd` output. It installs
only the required runtime packages from the fixed Ubuntu base and does not run
a blanket distribution upgrade. It runs as UID/GID `10001:10001` with
configuration expected at `/run/memochat/config.ini`.

## Current targets and local image names

| Target | Image |
| --- | --- |
| `ChatServer` | `memochat/chat-server:local` |
| `ChatDeliveryWorker` | `memochat/chat-delivery-worker:local` |
| `ChatRelationQueryService` | `memochat/chat-relation-query-service:local` |
| `ChatRelationServiceWorker` | `memochat/chat-relation-service-worker:local` |
| `ChatMessageService` | `memochat/chat-message-service:local` |
| `AIGatewayServer` | `memochat/ai-gateway:local` |
| `MediaGatewayServer` | `memochat/media-gateway:local` |
| `MomentsGatewayServer` | `memochat/moments-gateway:local` |
| `CallGatewayServer` | `memochat/call-gateway:local` |
| `R18GatewayServer` | `memochat/r18-gateway:local` |
| `RegisterServer` | `memochat/register-server:local` |
| `LoginServer` | `memochat/login-server:local` |
| `AccountServer` | `memochat/account-server:local` |
| `AIServer` | `memochat/ai-server:local` |
| `VarifyServer` | `memochat/varify-server:local` |

`GateServer` is not a current target. `VarifyServer` is a C++ executable and
uses this same runtime image; there is no separate Node/Varify Dockerfile.

## Runtime configuration

Mount one public INI read-only and inject credentials through environment
overrides. The entrypoint rejects an unknown service, a missing config, release
mode with `MEMOCHAT_ALLOW_DEV_SECRETS=1`, missing required variables, and common
placeholder/weak secret values. TLS private keys must be mounted read-only and
must never be placed in a bundle.

The image healthcheck confirms the service process remains alive. Protocol
readiness is checked by Compose/API probes because worker, gRPC, TCP/QUIC, and
HTTP targets do not share one health protocol.
