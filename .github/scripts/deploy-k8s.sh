#!/bin/bash
set -e

ENV="${1:-dev}"
NAMESPACE="memochat-${ENV}"
REGISTRY="${REGISTRY:-ghcr.io}"
IMAGE_NAME="${IMAGE_NAME:-memochat}"

if [ "$ENV" == "prod" ]; then
    NAMESPACE="memochat"
fi

echo "Deploying to $ENV environment..."
echo "Namespace: $NAMESPACE"

if ! command -v kubectl &> /dev/null; then
    echo "Error: kubectl is not installed"
    exit 1
fi

if ! command -v kustomize &> /dev/null; then
    echo "Installing kustomize..."
    kubectl kustomize > /dev/null 2>&1 || true
fi

echo "Applying K8s manifests..."
kubectl apply -k "infra/Memo_ops/k8s/overlays/${ENV}/" --namespace="$NAMESPACE"

echo "Waiting for deployments..."
kubectl rollout status deployment -n "$NAMESPACE" --timeout=300s || true
kubectl rollout status statefulset -n "$NAMESPACE" --timeout=300s || true

echo "Checking pods..."
kubectl get pods -n "$NAMESPACE"

echo "Deployment completed successfully!"
