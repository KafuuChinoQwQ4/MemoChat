# The caller must provide a fresh bundle produced by
# tools/scripts/release/package_backend_services.sh:
#   --build-context service_bundle=/absolute/path/to/bundles/<TARGET>
#   --build-context server_entrypoint=/absolute/path/to/entrypoint-context
ARG RUNTIME_IMAGE=ubuntu:24.04@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90
FROM service_bundle AS service_bundle
FROM server_entrypoint AS server_entrypoint

FROM ${RUNTIME_IMAGE}

ARG TARGET=ChatServer
ARG DEBIAN_FRONTEND=noninteractive

RUN case "${TARGET}" in \
      AIGatewayServer|AIServer|AccountServer|CallGatewayServer|ChatDeliveryWorker|ChatMessageService|ChatRelationQueryService|ChatRelationServiceWorker|ChatServer|LoginServer|MediaGatewayServer|MomentsGatewayServer|R18GatewayServer|RegisterServer|VarifyServer) ;; \
      *) echo "Unsupported TARGET: ${TARGET}" >&2; exit 64 ;; \
    esac \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      libturbojpeg0 \
      libwebp7 \
 && rm -rf /var/lib/apt/lists/* \
 && groupadd --gid 10001 memochat \
 && useradd --uid 10001 --gid memochat --no-create-home --home-dir /nonexistent --shell /usr/sbin/nologin memochat \
 && install -d -o memochat -g memochat -m 0750 /app/bin /app/lib /run/memochat

COPY --from=service_bundle --chown=memochat:memochat /bin/${TARGET} /app/bin/${TARGET}
COPY --from=service_bundle --chown=memochat:memochat /lib/ /app/lib/
COPY --from=service_bundle --chown=memochat:memochat /legal/ /app/legal/
COPY --from=service_bundle --chown=memochat:memochat /MANIFEST.txt /app/MANIFEST.txt
COPY --from=service_bundle --chown=memochat:memochat /SHA256SUMS /app/SHA256SUMS
COPY --from=server_entrypoint --chown=memochat:memochat /server-entrypoint.sh /app/entrypoint.sh

RUN set -eu; \
    grep -Fx "format=memochat-cpp-service-bundle-v1" /app/MANIFEST.txt >/dev/null; \
    grep -Fx "target=${TARGET}" /app/MANIFEST.txt >/dev/null; \
    test -x "/app/bin/${TARGET}"; \
    test -f /app/lib/libmsquic.so.2; \
    test -f /app/lib/libstdc++.so.6; \
    test -f /app/lib/libgcc_s.so.1; \
    test -f /app/lib/libatomic.so.1; \
    cd /app; \
    sha256sum --check --strict SHA256SUMS >/dev/null; \
    dependencies="$(LD_LIBRARY_PATH=/app/lib ldd "/app/bin/${TARGET}" 2>&1)"; \
    printf '%s\n' "${dependencies}"; \
    if printf '%s\n' "${dependencies}" | grep -Eq '=>[[:space:]]+not found|=>[[:space:]]+/(data|home|root|opt)/'; then \
      echo "Runtime dependency validation failed for ${TARGET}" >&2; \
      exit 1; \
    fi; \
    chmod 0555 /app/entrypoint.sh "/app/bin/${TARGET}"; \
    chmod 0444 /app/MANIFEST.txt /app/SHA256SUMS /app/lib/*; \
    find /app/legal -type f -exec chmod 0444 {} +

ENV MEMOCHAT_SERVICE=${TARGET} \
    MEMOCHAT_RELEASE_MODE=1 \
    MEMOCHAT_ALLOW_DEV_SECRETS=0 \
    CONFIG_PATH=/run/memochat/config.ini \
    LD_LIBRARY_PATH=/app/lib

WORKDIR /run/memochat
USER memochat

HEALTHCHECK --interval=20s --timeout=3s --start-period=10s --retries=3 \
    CMD ["/app/entrypoint.sh", "--healthcheck"]

ENTRYPOINT ["/app/entrypoint.sh"]
