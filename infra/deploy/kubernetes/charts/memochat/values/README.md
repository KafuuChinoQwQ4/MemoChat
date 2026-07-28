# Values Layout

- `../values.yaml`: base shared defaults for all environments
- `dev.yaml`: development overrides
- `staging.yaml`: staging overrides
- `prod.yaml`: production base overrides (calls, R18, and observability are off)
- `profiles/base.yaml`: explicit production base switch-off overlay
- `profiles/calls.yaml`: enables CallGateway and its relation-query dependency
- `profiles/r18.yaml`: enables R18Gateway while retaining encrypted persistence/key requirements
- `profiles/observability.yaml`: enables OTel and monitoring resources only

Examples:

```bash
helm upgrade --install memochat infra/deploy/kubernetes/charts/memochat \
  -f infra/deploy/kubernetes/charts/memochat/values/prod.yaml \
  -f infra/deploy/kubernetes/charts/memochat/values/profiles/base.yaml \
  --set externalSecrets.enabled=true \
  --set externalSecrets.relationTokensDistinct=true \
  --set envoy.tls.secretName=memochat-tls \
  --set images.releaseTag=sha-0123456789abcdef0123456789abcdef01234567
```

Select one or more optional profiles after the production base when needed;
they can be composed without enabling another capability implicitly:

```bash
# Calls only
helm upgrade --install memochat infra/deploy/kubernetes/charts/memochat \
  -f infra/deploy/kubernetes/charts/memochat/values/prod.yaml \
  -f infra/deploy/kubernetes/charts/memochat/values/profiles/base.yaml \
  -f infra/deploy/kubernetes/charts/memochat/values/profiles/calls.yaml \
  --set externalSecrets.enabled=true \
  --set externalSecrets.relationTokensDistinct=true \
  --set envoy.tls.secretName=memochat-tls \
  --set images.releaseTag=sha-0123456789abcdef0123456789abcdef01234567

# R18 only: persistence and r18-credential-master-key remain mandatory.
# Observability only: OTel/ServiceMonitor resources are enabled without Calls/R18.
```

Production rendering is fail-closed: use the exact `sha-<commit SHA>` manifest
tag emitted by CI. Do not use `latest` or a Docker Hub repository. When using
External Secrets, also set `externalSecrets.relationTokensDistinct=true` only
after auditing the four remote relation token values.
