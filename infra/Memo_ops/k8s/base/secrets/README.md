# P26: Vault/ExternalSecrets integration
# MemoChat — External Secrets Management

This directory contains the External Secrets Operator (ESO) configuration that
pulls runtime credentials from an external secret store into Kubernetes Secrets.

Two backends are documented:
- **AWS Secrets Manager** (primary, used by this base layer)
- **HashiCorp Vault** (alternative, used by the `overlays/prod` Vault overlay)

---

## 1. Install the External Secrets Operator

```bash
helm repo add external-secrets https://charts.external-secrets.io
helm repo update

helm install external-secrets external-secrets/external-secrets \
  --namespace external-secrets \
  --create-namespace \
  --set installCRDs=true \
  --version 0.9.x          # pin to a tested release
```

Verify the operator is running:

```bash
kubectl get pods -n external-secrets
kubectl get crds | grep external-secrets
```

---

## 2. AWS Secrets Manager (primary backend)

### 2a. IAM setup (IRSA — recommended for EKS)

Create the IAM policy:

```bash
cat > /tmp/memochat-sm-policy.json <<'EOF'
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "secretsmanager:GetSecretValue",
        "secretsmanager:DescribeSecret"
      ],
      "Resource": "arn:aws:secretsmanager:*:*:secret:memochat/*"
    }
  ]
}
EOF

aws iam create-policy \
  --policy-name MemoChat-SecretsManagerRead \
  --policy-document file:///tmp/memochat-sm-policy.json
```

Create the IAM role and bind it to the Kubernetes ServiceAccount (replace
`<ACCOUNT_ID>`, `<EKS_CLUSTER_NAME>`, and `<AWS_REGION>`):

```bash
eksctl create iamserviceaccount \
  --name external-secrets-sa \
  --namespace memochat \
  --cluster <EKS_CLUSTER_NAME> \
  --region <AWS_REGION> \
  --attach-policy-arn arn:aws:iam::<ACCOUNT_ID>:policy/MemoChat-SecretsManagerRead \
  --approve \
  --override-existing-serviceaccounts
```

### 2b. Store secrets in AWS Secrets Manager

Each secret is a JSON object stored at a dedicated path.  Use `--region` to
match the region configured in `external-secrets-store.yaml`.

```bash
REGION=us-east-1   # must match SecretStore .spec.provider.aws.region

# PostgreSQL
aws secretsmanager create-secret --region "$REGION" \
  --name memochat/postgres \
  --secret-string '{"password":"<STRONG_RANDOM_PASSWORD>"}'

# Redis
aws secretsmanager create-secret --region "$REGION" \
  --name memochat/redis \
  --secret-string '{"password":"<STRONG_RANDOM_PASSWORD>"}'

# RabbitMQ  (MEMOCHAT_RABBITMQ_PASSWORD)
aws secretsmanager create-secret --region "$REGION" \
  --name memochat/rabbitmq \
  --secret-string '{"username":"memochat","password":"<STRONG_RANDOM_PASSWORD>"}'

# MongoDB  (MEMOCHAT_MONGO_URI, MEMOCHAT_MONGODB_USER, MEMOCHAT_MONGODB_PASSWORD)
aws secretsmanager create-secret --region "$REGION" \
  --name memochat/mongodb \
  --secret-string '{
    "uri":      "mongodb://<user>:<pass>@<host>:27017/memochat?authSource=admin",
    "username": "<user>",
    "password": "<STRONG_RANDOM_PASSWORD>"
  }'

# Chat-auth HMAC secret  (MEMOCHAT_CHATAUTH_HMACSECRET — min 32 bytes, fail-closed)
aws secretsmanager create-secret --region "$REGION" \
  --name memochat/chat-auth \
  --secret-string "{\"hmac-secret\":\"$(openssl rand -base64 48)\"}"

# Refresh-token pepper  (MEMOCHAT_AUTH_REFRESH_PEPPER — min 32 bytes, fail-closed)
aws secretsmanager create-secret --region "$REGION" \
  --name memochat/refresh-pepper \
  --secret-string "{\"pepper\":\"$(openssl rand -base64 48)\"}"
```

To update an existing secret value:

```bash
aws secretsmanager put-secret-value --region "$REGION" \
  --secret-id memochat/postgres \
  --secret-string '{"password":"<NEW_PASSWORD>"}'
```

ESO will pick up the new value within the `refreshInterval` (1 h) or
immediately on next reconcile:

```bash
kubectl annotate externalsecret memochat-postgres-secret \
  force-sync="$(date +%s)" -n memochat --overwrite
```

### 2c. Patch the region per overlay

In `overlays/prod/kustomization.yaml` (or `overlays/staging/kustomization.yaml`),
add a strategic-merge patch to set the actual region:

```yaml
patches:
  - target:
      kind: SecretStore
      name: memochat-aws-secrets
    patch: |
      - op: replace
        path: /spec/provider/aws/region
        value: us-east-1
```

---

## 3. Alternative: HashiCorp Vault

The `overlays/prod` overlay already ships a `ClusterSecretStore` backed by an
internal Vault server using Kubernetes auth.  Use this section to provision the
corresponding secrets inside Vault.

### 3a. Enable KV v2 and Kubernetes auth (run once per Vault cluster)

```bash
vault secrets enable -path=secret kv-v2
vault auth enable kubernetes

vault write auth/kubernetes/config \
  kubernetes_host="https://<K8S_API_SERVER>" \
  kubernetes_ca_cert=@/var/run/secrets/kubernetes.io/serviceaccount/ca.crt \
  token_reviewer_jwt="$(cat /var/run/secrets/kubernetes.io/serviceaccount/token)"
```

### 3b. Create a policy and role for MemoChat

```bash
cat > /tmp/memochat-policy.hcl <<'EOF'
path "secret/data/memochat/prod/*" {
  capabilities = ["read"]
}
path "secret/metadata/memochat/prod/*" {
  capabilities = ["read", "list"]
}
EOF

vault policy write memochat-prod /tmp/memochat-policy.hcl

vault write auth/kubernetes/role/memochat-prod \
  bound_service_account_names=external-secrets \
  bound_service_account_namespaces=external-secrets \
  policies=memochat-prod \
  ttl=1h
```

### 3c. Store secrets in Vault

```bash
# Application secrets (maps to overlays/prod ExternalSecret memochat-secrets)
vault kv put secret/memochat/prod/app \
  postgres-password="<STRONG_RANDOM_PASSWORD>" \
  redis-password="<STRONG_RANDOM_PASSWORD>" \
  rabbitmq-username="memochat" \
  rabbitmq-password="<STRONG_RANDOM_PASSWORD>" \
  mongodb-uri="mongodb://<user>:<pass>@<host>:27017/memochat?authSource=admin" \
  chat-auth-secret="$(openssl rand -base64 48)" \
  auth-refresh-pepper="$(openssl rand -base64 48)" \
  neo4j-password="<STRONG_RANDOM_PASSWORD>" \
  smtp-pass="<SMTP_AUTH_CODE>" \
  livekit-api-key="<LIVEKIT_API_KEY>" \
  livekit-api-secret="<LIVEKIT_SECRET>" \
  minio-access-key="<MINIO_ACCESS_KEY>" \
  minio-secret-key="<MINIO_SECRET_KEY>" \
  ai-internal-api-key="<AI_INTERNAL_KEY>"

# Ops credentials (maps to ExternalSecret memochat-ops-secrets)
vault kv put secret/memochat/prod/ops \
  postgres-password="<OPS_POSTGRES_PASSWORD>" \
  redis-password="<OPS_REDIS_PASSWORD>"

# etcd root password (maps to ExternalSecret memochat-etcd-secrets)
vault kv put secret/memochat/prod/etcd \
  root-password="<ETCD_ROOT_PASSWORD>"

# etcd TLS (maps to ExternalSecret memochat-etcd-tls, type kubernetes.io/tls)
vault kv put secret/memochat/prod/etcd-tls \
  tls.crt="$(cat /path/to/etcd/tls.crt)" \
  tls.key="$(cat /path/to/etcd/tls.key)" \
  ca.crt="$(cat /path/to/etcd/ca.crt)"
```

### 3d. Staging Vault paths

For the staging overlay (once its `external-secrets.yaml` is added), mirror
the same structure under `memochat/staging/`:

```bash
vault policy write memochat-staging /tmp/memochat-staging-policy.hcl
# Policy should grant read on secret/data/memochat/staging/*

vault kv put secret/memochat/staging/app \
  postgres-password="<STAGING_PG_PASSWORD>" \
  # … same keys as prod
```

---

## 4. Verify secret sync

```bash
# Check SecretStore health
kubectl get secretstore memochat-aws-secrets -n memochat -o wide

# Check each ExternalSecret status
kubectl get externalsecrets -n memochat

# Inspect a synced Secret (values are base64-encoded)
kubectl get secret memochat-postgres-secret -n memochat -o jsonpath='{.data.postgres-password}' \
  | base64 -d && echo
```

A healthy ExternalSecret shows `READY=True` and `STATUS=SecretSynced`.
