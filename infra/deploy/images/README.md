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
tools/scripts/release/package_backend_services.sh \
  --library-dir "$VCPKG_ROOT/installed-memochat-gcc16-server-release/x64-linux-memochat-release/lib" \
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
  legal/                       # real root notices when available; otherwise empty
  MANIFEST.txt
  SHA256SUMS
```

The packaging step strips binaries, writes a relative RUNPATH, recursively
copies required compiler/non-system runtime libraries, and rejects unresolved
dependencies. Its initial and recursive `ldd` calls use a clean loader
environment: every non-system dependency must resolve below an explicit,
canonical `--library-dir`; ambient `LD_LIBRARY_PATH` and `LD_PRELOAD` values are
not trusted. `verify_release_tree.sh` is still mandatory: the Docker build does
not hide developer paths or suspicious content that remain in an ELF.

Generate the separately scanned, repository-shaped local deployment kit with:

```bash
tools/scripts/release/package_backend_deployment_kit.sh \
  --output /data/releases/memochat/backend-deployment-v1
```

It contains the fixed release Compose files/wrapper, sanitized public INI files,
all business migrations, and the PostgreSQL/MongoDB/MinIO provisioners. Normal
local packaging warns when root `LICENSE` or `THIRD_PARTY_NOTICES.md` is absent;
versioned tag CI fails closed until both real legal files exist. When present,
they are copied into the service bundles and deployment kit.

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
part of the build context.

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

The Dockerfile validates the target allowlist, manifest, checksums,
`libmsquic.so.2`, gcc16 runtime libraries, and final `ldd` output. It runs as
the unprivileged `memochat` user with configuration expected at
`/run/memochat/config.ini`.

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
