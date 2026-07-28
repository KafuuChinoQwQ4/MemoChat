# MemoChat Helm Chart

This chart deploys:

- optional legacy `GateServer` as a `Deployment` outside the production manifest
- `ChatServer` as a `StatefulSet`
- `ChatRelationQueryService` as an internal gRPC `Deployment` and `Service`
- `ChatRelationServiceWorker` as the authenticated internal relation-command gRPC service
- `VarifyServer` as a `Deployment`
- optional `Memo_ops` server and collector outside the production manifest
- `otel-collector`

It assumes PostgreSQL, Redis, MongoDB, Kafka, RabbitMQ, MySQL, and LiveKit are external services.

## Use release images

The chart must consume the immutable images produced by the release workflow.
Build the 15 service images from fresh bundles with the release tooling; this
Dockerfile intentionally cannot build from the repository root:

```bash
tools/scripts/release/build_backend_images.sh \
  --bundle-root /data/releases/memochat/backend \
  --image-prefix ghcr.io/ORG/memochat \
  --tag sha-<commit-sha>
```

For a formal release, set chart image tags to the CI-produced commit digest
and verify the release manifest before deploying. Never use `latest` or the
removed `GateServer` target.

## Install

```bash
helm upgrade --install memochat infra/deploy/kubernetes/charts/memochat \
  -f infra/deploy/kubernetes/charts/memochat/values/prod.yaml \
  --set externalSecrets.enabled=true \
  --set externalSecrets.relationTokensDistinct=true \
  --set envoy.tls.secretName=memochat-tls \
  --set images.releaseTag=sha-0123456789abcdef0123456789abcdef01234567
```

## Notes

- Every chart-managed Deployment, StatefulSet, and bootstrap Job renders pod and
  container security contexts for UID/GID `10001`, `RuntimeDefault` seccomp,
  `allowPrivilegeEscalation: false`, `readOnlyRootFilesystem: true`, and
  `capabilities.drop: [ALL]`. Writable state is explicit: `/tmp` and service
  logs use `emptyDir`, local media uses dedicated upload mounts, AI runtime
  state uses `/app/.cache` and `/app/.data`, and R18 uses its encrypted PVC at
  `/run/memochat/data/r18`. No chart workload uses `hostPath`.
- `ChatServer` discovery uses `k8s-statefulset`.
- `CallGatewayServer` requires the internal relation-query service and the chart fails closed if that dependency is disabled.
- Relation gRPC traffic uses distinct command, Chat query, Call query, and
  Moments query authentication tokens. Supply all four
  `secrets.relation*AuthToken` values with at least 32 printable ASCII bytes
  each, or provide the matching `relation-*-auth-token` properties through
  External Secrets; never reuse a token between roles.
- Enabling either relation gRPC service (including Call, Moments, or remote Chat
  command/query consumers) requires the Istio mesh with `PeerAuthentication` mode `STRICT`;
  chart rendering fails when that transport encryption boundary is absent.
- With External Secrets enabled, set `externalSecrets.relationTokensDistinct=true`
  only after checking the four Vault token properties for printable ASCII,
  minimum length, and pairwise uniqueness; Helm cannot inspect remote secret
  values and therefore fails closed on a missing attestation.
- `GateServer` now exposes `/healthz` and `/readyz`.
- Production values enable Istio `PeerAuthentication` `STRICT` by default and
  require `images.releaseTag=sha-<40 lowercase hex>`; mutable `latest` tags and
  non-GHCR C++ repositories fail Helm rendering. The release tag is the CI
  manifest tag for the commit being deployed.
- PostgreSQL business workloads use role-specific Secret keys (`postgres-chat`,
  `postgres-call`, `postgres-account`, and the media/moments equivalents). The
  generic `postgres-password` key is reserved for migrations and administrative
  components.
- Envoy and OpenTelemetry Collector are explicit infrastructure exceptions and
  are pinned by `sha256` digest. Migration/bootstrap Job images must use named
  version tags and are rejected if configured as `latest`.
- Legacy GateServer, AI Orchestrator, and MemoOps are not part of the 15-image
  manifest and are disabled by the production values until separately attested
  immutable images are supplied. Bootstrap Job images are also required to use
  exact `repository@sha256:<digest>` references in production.
- `VarifyServer` exposes HTTP health checks on a separate port.
- The chart uses hook Jobs for PostgreSQL schema bootstrap and Kafka/RabbitMQ topology initialization.
- `templates/` is grouped into `prod`, `ops`, `observability`, `bootstrap`, and `shared`.

## Optional production profiles

`values/prod.yaml` is the production base: `CallGateway`, `R18Gateway`, and
the OTel/monitoring resources are disabled. Layer the explicit base profile and
at most the capability profiles required by the deployment:

```bash
BASE=infra/deploy/kubernetes/charts/memochat/values/prod.yaml
PROFILE=infra/deploy/kubernetes/charts/memochat/values/profiles/base.yaml

helm upgrade --install memochat infra/deploy/kubernetes/charts/memochat \
  -f "$BASE" -f "$PROFILE" \
  -f infra/deploy/kubernetes/charts/memochat/values/profiles/calls.yaml \
  --set externalSecrets.enabled=true \
  --set externalSecrets.relationTokensDistinct=true \
  --set envoy.tls.secretName=memochat-tls \
  --set images.releaseTag=sha-0123456789abcdef0123456789abcdef01234567
```

Use `profiles/r18.yaml` and/or `profiles/observability.yaml` alongside the
Calls profile when those capabilities are needed. Each profile controls only
its own optional workload; R18 keeps its encrypted persistent volume and
master-key Secret reference, while Calls explicitly enables the relation-query
dependency.
