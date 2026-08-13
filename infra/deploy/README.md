# Deploy Layout

This directory is split by deployment concern instead of by tool name.

## Structure

- `images/`
  - Container build inputs
  - `services/` contains service-specific Dockerfiles
  - `common/entrypoints/` contains shared runtime entrypoints
- `local/`
  - Local Docker Compose files for development dependencies
- `kubernetes/`
  - Kubernetes-facing assets
  - `charts/` contains Helm charts
  - Chart templates are grouped by subsystem

## Common Entry Points

Build the release images from fresh, verified service bundles. The Dockerfile
expects the commit-bound vcpkg SPDX produced by the packager; do not build a
service directly from the repository or use mutable `latest` tags:

```bash
tools/scripts/release/package_backend_services.sh \
  --vcpkg-installed-root "$VCPKG_ROOT/installed-memochat-gcc16-server-release" \
  --vcpkg-triplet x64-linux-memochat-release \
  --output /data/releases/memochat/backend
tools/scripts/release/build_backend_images.sh \
  --bundle-root /data/releases/memochat/backend \
  --image-prefix memochat \
  --tag local
```

For a formal versioned release, use the tag-triggered CI artifacts and their
immutable `sha-<commit>` references; the workflow is the only path that writes
GHCR or creates version aliases.

Install chart:

```bash
helm upgrade --install memochat deploy/kubernetes/charts/memochat \
  -f deploy/kubernetes/charts/memochat/values/prod.yaml
```

Start local dependency containers:

```bash
docker compose -f deploy/local/compose/kafka.yml up -d
docker compose -f deploy/local/compose/rabbitmq.yml up -d
docker compose -f deploy/local/compose/livekit.yml up -d
```
