#!/usr/bin/env bash
set -Eeuo pipefail

ENV="${1:-dev}"
NAMESPACE="memochat-${ENV}"

case "$ENV" in
    dev|dev-single|staging)
        ;;
    prod)
        echo "Error: the legacy MemoOps Kustomize production path is retired." >&2
        echo "Use infra/deploy/kubernetes/charts/memochat with reviewed production values and External Secrets." >&2
        exit 64
        ;;
    *)
        echo "Error: unsupported environment: $ENV" >&2
        exit 64
        ;;
esac

echo "Deploying to $ENV environment..."
echo "Namespace: $NAMESPACE"

if ! command -v kubectl >/dev/null 2>&1; then
    echo "Error: kubectl is not installed"
    exit 1
fi

echo "Applying K8s manifests..."
kubectl apply -k "infra/Memo_ops/k8s/overlays/${ENV}/" --namespace="$NAMESPACE"

echo "Waiting for deployments..."
kubectl rollout status deployment -n "$NAMESPACE" --timeout=300s || true
kubectl rollout status statefulset -n "$NAMESPACE" --timeout=300s || true

echo "Checking pods..."
kubectl get pods -n "$NAMESPACE"

echo "Deployment completed successfully!"
