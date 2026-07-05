# H-12: mTLS between all MemoChat microservices

Istio service-mesh configuration for MemoChat. All files in this directory
are kustomize base resources consumed by each overlay (`dev`, `staging`, `prod`).

---

## Prerequisites

- Kubernetes 1.27+
- `istioctl` 1.20+ on your PATH
- `kubectl` configured for the target cluster

---

## 1. Install Istio

Install Istio with the default profile (includes the control plane and
ingress gateway):

```bash
istioctl install --set profile=default -y
```

Verify the control-plane pods are running:

```bash
kubectl get pods -n istio-system
```

Expected output includes `istiod-*` (Running) and `istio-ingressgateway-*`
(Running).

---

## 2. Label the namespace for sidecar injection

The Envoy sidecar is injected automatically when the namespace carries the
`istio-injection=enabled` label. The prod overlay applies this label via a
kustomize patch; for other environments run it manually:

```bash
# Production (managed by kustomize patch — no manual step required)

# Staging
kubectl label namespace memochat-staging istio-injection=enabled --overwrite

# Dev
kubectl label namespace memochat-dev istio-injection=enabled --overwrite
```

Restart existing deployments so pods are re-created with the sidecar:

```bash
kubectl rollout restart deployment -n memochat
```

---

## 3. Apply the mesh configuration

The base manifests in this directory are applied through kustomize overlays.
Direct application is also possible for testing:

```bash
# Apply base Istio resources directly (namespace must already exist)
kubectl apply -f infra/Memo_ops/k8s/base/istio/peer-authentication.yaml
kubectl apply -f infra/Memo_ops/k8s/base/istio/destination-rules.yaml
```

Or via the overlay (recommended):

```bash
kubectl apply -k infra/Memo_ops/k8s/overlays/prod
```

---

## 4. Verify mTLS

### Check TLS status for all services in the namespace

```bash
istioctl authn tls-check -n memochat
```

Every row should show `OK` under the `STATUS` column and `mTLS` under
`SERVER` and `CLIENT`.

### Inspect a specific service

```bash
istioctl authn tls-check memochat-chatserver.memochat.svc.cluster.local \
  -n memochat
```

### Confirm PeerAuthentication is active

```bash
kubectl get peerauthentication -n memochat
# NAME                    MODE     AGE
# memochat-strict-mtls    STRICT   ...
```

### Confirm DestinationRules are active

```bash
kubectl get destinationrule -n memochat
```

### Check the proxy config of a running pod

```bash
POD=$(kubectl get pod -n memochat -l app=memochat-chatserver \
      -o jsonpath='{.items[0].metadata.name}')
istioctl proxy-config cluster "$POD" -n memochat | grep memochat
```

---

## 5. Staging overlay — PERMISSIVE mode during roll-out

During the initial mesh roll-out in staging, add a kustomize patch to the
staging overlay to use PERMISSIVE mode until all sidecars are confirmed
healthy, then switch to STRICT:

```yaml
# infra/Memo_ops/k8s/overlays/staging/mesh-istio.yaml
# H-12: mTLS between all MemoChat microservices
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: memochat-strict-mtls
  namespace: memochat-staging
spec:
  mtls:
    mode: PERMISSIVE   # change to STRICT once all pods have sidecars
```

Add `mesh-istio.yaml` to `overlays/staging/kustomization.yaml` under
`resources:` and add a patch that sets `namespace: memochat-staging`.

---

## 6. Dev overlays

Dev overlays (`dev` and `dev-single`) do not enable Istio injection by
default to reduce resource overhead on local clusters. To opt a dev
environment in:

1. Label the namespace (see step 2 above).
2. Apply the base peer-authentication with `mode: PERMISSIVE` to avoid
   breaking pods that run without sidecars.

---

## 7. Egress to external services

External endpoints (Vault, LiveKit, MinIO, managed databases) require
`SIMPLE` or `MUTUAL` TLS DestinationRules with a `ServiceEntry`. Example
for Vault:

```yaml
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: vault-external
  namespace: memochat
spec:
  hosts:
    - vault.example.internal
  ports:
    - number: 8200
      name: https
      protocol: HTTPS
  location: MESH_EXTERNAL
  resolution: DNS
---
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: vault-external
  namespace: memochat
spec:
  host: vault.example.internal
  trafficPolicy:
    tls:
      mode: SIMPLE
      sni: vault.example.internal
```

Add similar `ServiceEntry` + `DestinationRule` pairs for each external
endpoint and place them under `infra/Memo_ops/k8s/base/istio/egress/`.

---

## File inventory

| File | Purpose |
|---|---|
| `peer-authentication.yaml` | Namespace-wide STRICT mTLS enforcement |
| `destination-rules.yaml` | Per-service ISTIO_MUTUAL client TLS |
| `README.md` | This file |

---

## References

- [Istio PeerAuthentication](https://istio.io/latest/docs/reference/config/security/peer_authentication/)
- [Istio DestinationRule](https://istio.io/latest/docs/reference/config/networking/destination-rule/)
- [Istio mutual TLS migration](https://istio.io/latest/docs/tasks/security/authentication/mtls-migration/)
