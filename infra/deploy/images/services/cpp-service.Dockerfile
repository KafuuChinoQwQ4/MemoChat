# The caller must provide a fresh bundle produced by
# tools/scripts/release/package_backend_services.sh:
#   --build-context service_bundle=/absolute/path/to/bundles/<TARGET>
#   --build-context server_entrypoint=/absolute/path/to/entrypoint-context
ARG RUNTIME_IMAGE=ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90
FROM service_bundle AS service_bundle
FROM server_entrypoint AS server_entrypoint

# The minimal Ubuntu base does not contain a CA bundle. Bootstrap only the
# certificate data from the exact package that the fixed snapshot will install;
# the final stage still installs and records the package through apt/dpkg.
FROM ${RUNTIME_IMAGE} AS apt_ca_bootstrap

ARG CA_CERTIFICATES_DEB_URL=https://snapshot.ubuntu.com/ubuntu/20260727T000000Z/pool/main/c/ca-certificates/ca-certificates_20260601~24.04.1_all.deb

ADD --checksum=sha256:6bac2a01979e210d9eac1d4d56747ec709ea60654744d66705dc3c36e7629e50 \
    ${CA_CERTIFICATES_DEB_URL} \
    /tmp/ca-certificates.deb

RUN set -eu; \
    mkdir -p /tmp/ca-certificates /etc/ssl/certs; \
    dpkg-deb --extract /tmp/ca-certificates.deb /tmp/ca-certificates; \
    cat /tmp/ca-certificates/usr/share/ca-certificates/mozilla/*.crt \
      > /etc/ssl/certs/ca-certificates.crt; \
    test -s /etc/ssl/certs/ca-certificates.crt

FROM ${RUNTIME_IMAGE}

ARG RUNTIME_IMAGE
ARG MEMOCHAT_BASE_DIGEST=sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90
ARG UBUNTU_SNAPSHOT=20260727T000000Z

LABEL io.memochat.base.reference="${RUNTIME_IMAGE}" \
      io.memochat.base.digest="${MEMOCHAT_BASE_DIGEST}" \
      io.memochat.ubuntu.snapshot="${UBUNTU_SNAPSHOT}" \
      io.memochat.ubuntu.ca-bootstrap.sha256="6bac2a01979e210d9eac1d4d56747ec709ea60654744d66705dc3c36e7629e50" \
      io.memochat.ubuntu.runtime-packages="ca-certificates=20260601~24.04.1,libturbojpeg=1:2.1.5-2ubuntu2,libwebp7=1.3.2-0.4build3"

ARG DEBIAN_FRONTEND=noninteractive

COPY --from=apt_ca_bootstrap --chmod=0644 \
    /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt

RUN --mount=type=secret,id=memochat_builder_ca,required=false,mode=0444 \
    test "${RUNTIME_IMAGE##*@}" = "${MEMOCHAT_BASE_DIGEST}" \
 && test "$UBUNTU_SNAPSHOT" = 20260727T000000Z \
 && printf '%s' "${UBUNTU_SNAPSHOT}" | grep -Eq '^[0-9]{8}T[0-9]{6}Z$' \
 && sed -i \
      "s#^URIs: .*#URIs: https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}#" \
      /etc/apt/sources.list.d/ubuntu.sources \
 && sed -i \
      -e 's/^Suites: noble noble-updates noble-backports$/Suites: noble noble-updates/' \
      -e 's/^Components: .*$/Components: main universe/' \
      /etc/apt/sources.list.d/ubuntu.sources \
 && sed -i '/^Signed-By:/a Check-Valid-Until: no' \
      /etc/apt/sources.list.d/ubuntu.sources \
 && test "$(grep -c "^URIs: https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}$" \
      /etc/apt/sources.list.d/ubuntu.sources)" -eq 2 \
 && test "$(grep -c '^Check-Valid-Until: no$' \
      /etc/apt/sources.list.d/ubuntu.sources)" -eq 2 \
 && ! grep -Eq '^URIs: http://(archive|security)\.ubuntu\.com/ubuntu/?$' \
      /etc/apt/sources.list.d/ubuntu.sources \
 && ! grep -Eq '^(Suites: .*noble-backports|Components: .*(restricted|multiverse))$' \
      /etc/apt/sources.list.d/ubuntu.sources \
 && apt_ca_option='' \
 && if [ -s /run/secrets/memochat_builder_ca ]; then \
      apt_ca_option='-o Acquire::https::CaInfo=/run/secrets/memochat_builder_ca'; \
    fi \
 && apt-get -o APT::Update::Error-Mode=any -o Acquire::Retries=3 ${apt_ca_option} update \
 && apt-get ${apt_ca_option} install -y --no-install-recommends \
      ca-certificates \
      libturbojpeg \
      libwebp7 \
 && test "$(dpkg-query -W ca-certificates | cut -f2)" = '20260601~24.04.1' \
 && test "$(dpkg-query -W libturbojpeg | cut -f2)" = '1:2.1.5-2ubuntu2' \
 && test "$(dpkg-query -W libwebp7 | cut -f2)" = '1.3.2-0.4build3' \
 && test -x /usr/bin/bash \
 && test -x /usr/bin/timeout \
 && rm -rf /var/lib/apt/lists/* \
 && groupadd --gid 10001 memochat \
 && useradd --uid 10001 --gid memochat --no-create-home --home-dir /nonexistent --shell /usr/sbin/nologin memochat \
 && install -d -o memochat -g memochat -m 0750 /app/bin /app/lib /run/memochat

ARG TARGET=ChatServer

RUN case "${TARGET}" in \
      AIGatewayServer|AIServer|AccountServer|CallGatewayServer|ChatDeliveryWorker|ChatMessageService|ChatRelationQueryService|ChatRelationServiceWorker|ChatServer|LoginServer|MediaGatewayServer|MomentsGatewayServer|R18GatewayServer|RegisterServer|VarifyServer) ;; \
      *) echo "Unsupported TARGET: ${TARGET}" >&2; exit 64 ;; \
    esac

COPY --from=service_bundle --chown=memochat:memochat /bin/${TARGET} /app/bin/${TARGET}
COPY --from=service_bundle --chown=memochat:memochat /lib/ /app/lib/
COPY --from=service_bundle --chown=memochat:memochat /legal/ /app/legal/
COPY --from=service_bundle --chown=memochat:memochat /sbom/ /app/sbom/
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
    test -s /app/sbom/vcpkg-build-dependencies.spdx.json; \
    grep -Fx "vcpkg_sbom_coverage=installed-closure-overapproximation" /app/MANIFEST.txt >/dev/null; \
    expected_vcpkg_sbom_sha256="$(sed -n 's/^vcpkg_sbom_sha256=//p' /app/MANIFEST.txt)"; \
    test "$(printf '%s' "${expected_vcpkg_sbom_sha256}" | wc -c)" -eq 64; \
    printf '%s  %s\n' "${expected_vcpkg_sbom_sha256}" /app/sbom/vcpkg-build-dependencies.spdx.json \
      | sha256sum --check --strict - >/dev/null; \
    cd /app; \
    sha256sum --check --strict SHA256SUMS >/dev/null; \
    dependencies="$(LD_LIBRARY_PATH=/app/lib ldd "/app/bin/${TARGET}" 2>&1)"; \
    printf '%s\n' "${dependencies}"; \
    if printf '%s\n' "${dependencies}" | grep -Eq '=>[[:space:]]+not found|=>[[:space:]]+/(data|home|root|opt)/'; then \
      echo "Runtime dependency validation failed for ${TARGET}" >&2; \
      exit 1; \
    fi; \
    chmod 0555 /app/entrypoint.sh "/app/bin/${TARGET}"; \
    chmod 0444 /app/MANIFEST.txt /app/SHA256SUMS /app/lib/* /app/sbom/*; \
    find /app/legal -type f -exec chmod 0444 {} +

ENV MEMOCHAT_SERVICE=${TARGET} \
    MEMOCHAT_RELEASE_MODE=1 \
    MEMOCHAT_ALLOW_DEV_SECRETS=0 \
    CONFIG_PATH=/run/memochat/config.ini \
    LD_LIBRARY_PATH=/app/lib

WORKDIR /run/memochat
USER 10001:10001

HEALTHCHECK --interval=20s --timeout=3s --start-period=10s --retries=3 \
    CMD ["/app/entrypoint.sh", "--healthcheck"]

ENTRYPOINT ["/app/entrypoint.sh"]
