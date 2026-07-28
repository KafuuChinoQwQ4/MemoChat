{{- define "memochat.fullname" -}}
{{- printf "%s-%s" .Release.Name .Chart.Name | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "memochat.labels" -}}
app.kubernetes.io/name: {{ .Chart.Name }}
app.kubernetes.io/instance: {{ .Release.Name }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
helm.sh/chart: {{ printf "%s-%s" .Chart.Name .Chart.Version | quote }}
{{- end -}}

{{- define "memochat.selectorLabels" -}}
app.kubernetes.io/name: {{ .Chart.Name }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end -}}

{{- define "memochat.validateProductionImages" -}}
{{- if eq (default "development" .Values.global.environment) "production" -}}
{{- if .Values.externalSecrets.enabled -}}
{{- if not .Values.externalSecrets.relationTokensDistinct -}}
{{- fail "externalSecrets.relationTokensDistinct=true is required after auditing all four Vault relation token properties" -}}
{{- end -}}
{{- end -}}
{{- if .Values.legacyGate.enabled -}}
{{- fail "legacyGate.enabled=true is not supported by the 15-image release manifest; disable the legacy GateServer workload" -}}
{{- end -}}
{{- if .Values.aiOrchestrator.enabled -}}
{{- fail "aiOrchestrator.enabled=true requires a separately attested immutable image; it is not part of the 15-image release manifest" -}}
{{- end -}}
{{- if .Values.memoOps.enabled -}}
{{- fail "memoOps.enabled=true requires a separately attested immutable image; it is not part of the 15-image release manifest" -}}
{{- end -}}
{{- $registry := required "images.releaseRegistry is required in production" .Values.images.releaseRegistry | toString -}}
{{- if not (regexMatch "^ghcr\\.io/[a-z0-9][a-z0-9-]*/memochat$" $registry) -}}
{{- fail "images.releaseRegistry must be the CI GHCR repository ghcr.io/<owner>/memochat" -}}
{{- end -}}
{{- $tag := required "images.releaseTag must be set to the CI commit manifest tag in production" .Values.images.releaseTag | toString -}}
{{- if not (regexMatch "^sha-[0-9a-f]{40}$" $tag) -}}
{{- fail "images.releaseTag must match sha-<40 lowercase hex commit SHA>; mutable tags are rejected" -}}
{{- end -}}
{{- range $entry := list
  (dict "name" "images.envoy" "image" .Values.images.envoy)
  (dict "name" "images.otelCollector" "image" .Values.images.otelCollector)
}}
{{- if not (regexMatch "^sha256:[0-9a-f]{64}$" (toString $entry.image.digest)) -}}
{{- fail (printf "%s.digest must be a pinned sha256 digest in production" $entry.name) -}}
{{- end -}}
{{- end -}}
{{- range $job := list
  (dict "name" "jobs.postgresMigrate.image" "value" .Values.jobs.postgresMigrate.image)
  (dict "name" "jobs.kafkaInit.image" "value" .Values.jobs.kafkaInit.image)
  (dict "name" "jobs.rabbitmqInit.image" "value" .Values.jobs.rabbitmqInit.image)
}}
{{- if not (regexMatch "^.+@sha256:[0-9a-f]{64}$" (toString $job.value)) -}}
{{- fail (printf "%s must use an immutable repository@sha256 digest in production" $job.name) -}}
{{- end -}}
{{- end -}}
{{- $releaseImages := list
  (dict "name" "images.focusedGateways.aigateway" "image" .Values.images.focusedGateways.aigateway "slug" "ai-gateway")
  (dict "name" "images.focusedGateways.mediagateway" "image" .Values.images.focusedGateways.mediagateway "slug" "media-gateway")
  (dict "name" "images.focusedGateways.momentsgateway" "image" .Values.images.focusedGateways.momentsgateway "slug" "moments-gateway")
  (dict "name" "images.focusedGateways.callgateway" "image" .Values.images.focusedGateways.callgateway "slug" "call-gateway")
  (dict "name" "images.focusedGateways.r18gateway" "image" .Values.images.focusedGateways.r18gateway "slug" "r18-gateway")
  (dict "name" "images.focusedGateways.register" "image" .Values.images.focusedGateways.register "slug" "register-server")
  (dict "name" "images.focusedGateways.login" "image" .Values.images.focusedGateways.login "slug" "login-server")
  (dict "name" "images.focusedGateways.account" "image" .Values.images.focusedGateways.account "slug" "account-server")
  (dict "name" "images.chat" "image" .Values.images.chat "slug" "chat-server")
  (dict "name" "images.chatRelationQuery" "image" .Values.images.chatRelationQuery "slug" "chat-relation-query-service")
  (dict "name" "images.chatRelationService" "image" .Values.images.chatRelationService "slug" "chat-relation-service-worker")
  (dict "name" "images.aiServer" "image" .Values.images.aiServer "slug" "ai-server")
  (dict "name" "images.varify" "image" .Values.images.varify "slug" "varify-server")
}}
{{- range $entry := $releaseImages -}}
{{- $expected := printf "%s/%s" $registry $entry.slug -}}
{{- if ne $entry.image.repository $expected -}}
{{- fail (printf "%s.repository must be %s in production" $entry.name $expected) -}}
{{- end -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{- define "memochat.image" -}}
{{- $root := .root -}}
{{- $image := .image -}}
{{- $tag := default $image.tag $root.Values.images.releaseTag -}}
{{- printf "%s:%s" $image.repository $tag -}}
{{- end -}}

{{- define "memochat.infrastructureImage" -}}
{{- $image := . -}}
{{- if $image.digest -}}
{{- printf "%s@%s" $image.repository $image.digest -}}
{{- else -}}
{{- printf "%s:%s" $image.repository $image.tag -}}
{{- end -}}
{{- end -}}

{{/* Every chart-managed workload runs with the release UID and pod-level seccomp. */}}
{{- define "memochat.podSecurityContext" -}}
runAsNonRoot: true
runAsUser: 10001
runAsGroup: 10001
fsGroup: 10001
fsGroupChangePolicy: OnRootMismatch
seccompProfile:
  type: RuntimeDefault
{{- end -}}

{{/* Keep the root filesystem immutable; writable state is mounted explicitly per workload. */}}
{{- define "memochat.containerSecurityContext" -}}
privileged: false
allowPrivilegeEscalation: false
readOnlyRootFilesystem: true
runAsNonRoot: true
runAsUser: 10001
runAsGroup: 10001
seccompProfile:
  type: RuntimeDefault
capabilities:
  drop:
    - ALL
{{- end -}}
