# Third-Party Notices

MemoChat is licensed under the MIT License; see `LICENSE`. Components listed
below remain under their respective licenses.

## Scope And Review Status

This inventory was prepared from the Linux release packaging scripts, the
current local Linux client artifact, all 15 C++ service linker commands, the
installed vcpkg SPDX records, the Qt 6.8.3 SPDX records, and the release
container definitions. It distinguishes software that MemoChat distributes
from software obtained separately by an operator.

This file is a reviewable inventory, but it is not a substitute for the
verbatim license, copyright, NOTICE, source-offer, and relinking materials
required by individual licenses. Those materials are not yet all present in
the release layout. The section [Required Materials Before Formal Release](#required-materials-before-formal-release)
records the remaining work without representing it as complete.

The final commit-SHA artifacts and image digests are authoritative. Their SBOMs
must be compared with this inventory because recursive library copying, Qt
modules, compiler packages, and Ubuntu packages can change independently of
the source manifests.

`tools/scripts/release/verify_release_legal.sh` treats this inventory and the
formal distribution corpus as separate states. Ordinary CI and local packages
may report `third_party_legal_corpus=incomplete`, but a version tag must require
`legal/third-party/CORPUS.json`, `SHA256SUMS`, all required distribution scopes,
an explicit `approved-for-distribution` review status, and a stable `review_id`.
A present but partial or checksum-invalid corpus is an error even outside a
versioned release. Formal approval is a separate external signature over a
canonical payload generated only from an exact clean source commit. This gate
records reviewed completeness; it does not replace legal review of the materials.

The repository cannot approve its own legal corpus. `CORPUS.json` review fields
are inputs to the approval payload, not a trust root. After the release source
commit is final, the verifier writes a canonical
`memochat-release-legal-approval-v2` payload outside the repository. That payload
binds the exact source commit and Git tree, `LICENSE` and this notice by SHA-256,
the corpus review ID, and the corpus `SHA256SUMS` digest. The legal approver signs
those exact bytes with an OpenSSL SHA-256 detached signature. The payload,
signature, signing private key, and approval public key all remain outside the
repository. Version-tag CI materializes only the externally supplied payload
signature and public key outside the checkout and passes their paths explicitly.
The verifier rejects either input when it is a symlink or resolves anywhere
under the repository root.

`CORPUS.json` uses schema `memochat-third-party-corpus-v1` and contains only
`schema`, `review_status`, `review_id`, and `scopes`. The required scope keys are
`qt`, `qtwebengine-chromium`, `ffmpeg`, `icu`, `gcc-runtime`, `backend-vcpkg`,
`container-ubuntu`, and `client-assets`. Every scope must list at least one
nonempty regular file; every payload and `CORPUS.json` must appear exactly once
in `SHA256SUMS`, and undeclared files, unsafe paths, symlinks, and checksum
mismatches are rejected. Repository-owned `CORPUS.sig` files are undeclared and
rejected. `LEGAL-STATUS.txt` records the canonical approval payload digest,
external signature digest, normalized public-key fingerprint, exact source SHA
and tree, and verification result. Only a verified external signature over that
exact-source payload can set `formal_distribution_ready=true`.

After committing the complete corpus, use the following four-step approval flow.
The detached signature is commit-specific and must be regenerated after any
source, tree, license, notice, review ID, or corpus checksum change.

1. In the exact clean release checkout, generate the canonical payload directly
   to a new path outside the repository. Record its SHA-256 through a separately
   authenticated channel for the approver:

```bash
approval_payload=/secure/external/memochat-legal-approval-v2.txt
release_source_sha="$(git rev-parse --verify HEAD)"

tools/scripts/release/verify_release_legal.sh \
  --source-sha "$release_source_sha" \
  --write-approval-payload "$approval_payload"

sha256sum -- "$approval_payload"
```

2. Transfer only that payload into the isolated/offline signing environment,
   confirm its SHA-256 against the authenticated value, and sign those exact
   bytes. The private key remains offline; export only the detached signature
   and matching public key:

```bash
approval_payload=/offline/inbox/memochat-legal-approval-v2.txt
approval_signature=/offline/outbox/memochat-legal-approval-v2.sig
signing_key_path=/offline/keys/memochat-legal-approval-private.pem
approval_public_key=/offline/outbox/memochat-legal-approval-public.pem

sha256sum -- "$approval_payload"
openssl dgst -sha256 \
  -sign "$signing_key_path" \
  -out "$approval_signature" \
  "$approval_payload"
openssl pkey -in "$signing_key_path" -pubout -out "$approval_public_key"
openssl dgst -sha256 \
  -verify "$approval_public_key" \
  -signature "$approval_signature" \
  "$approval_payload"
```

3. On the connected release-administration machine, base64-encode the binary
   signature as one line and configure these exact protected GitHub Actions
   secrets. `MEMOCHAT_LEGAL_APPROVAL_PUBLIC_KEY_PEM` contains the complete PEM
   public key; `MEMOCHAT_LEGAL_APPROVAL_SIGNATURE_BASE64` contains the detached
   signature bytes encoded as base64. Neither secret contains the private key:

```bash
approval_signature=/secure/import/memochat-legal-approval-v2.sig
approval_public_key=/secure/import/memochat-legal-approval-public.pem
signature_base64=/secure/import/memochat-legal-approval-v2.sig.base64

base64 --wrap=0 "$approval_signature" > "$signature_base64"
gh secret set MEMOCHAT_LEGAL_APPROVAL_PUBLIC_KEY_PEM < "$approval_public_key"
gh secret set MEMOCHAT_LEGAL_APPROVAL_SIGNATURE_BASE64 < "$signature_base64"
```

4. Before creating a version tag, CI materializes the protected public key and
   detached signature under `$RUNNER_TEMP`, outside `$GITHUB_WORKSPACE`, and
   invokes the verifier as follows:

```bash
tools/scripts/release/verify_release_legal.sh \
  --require-distribution-corpus \
  --source-sha "$GITHUB_SHA" \
  --approval-public-key "$RUNNER_TEMP/memochat-legal-approval-public.pem" \
  --approval-signature "$RUNNER_TEMP/memochat-legal-approval.sig"
```

## Linux Client Distribution

The Linux client archive includes the MemoChat executable, Qt libraries,
plugins, QML modules and translations, Qt WebEngine resources and helper
process, and recursively copied non-system runtime libraries.

### Direct Runtime Components

| Component | Confirmed version | License used for distribution | Upstream | Distribution role |
| --- | --- | --- | --- | --- |
| Qt Base | 6.8.3 | LGPL-3.0-only elected from Qt's available license alternatives; bundled code has separate licenses | https://code.qt.io/cgit/qt/qtbase.git/ | Dynamic Qt libraries and platform, image, TLS, SQL, and related plugins |
| Qt Declarative | 6.8.3 | LGPL-3.0-only elected; bundled code has separate licenses | https://code.qt.io/cgit/qt/qtdeclarative.git/ | QML, Qt Quick, Controls, Dialogs, Effects, Layouts, Shapes, Templates, and Window modules |
| Qt Multimedia | 6.8.3 | LGPL-3.0-only elected; bundled code has separate licenses | https://code.qt.io/cgit/qt/qtmultimedia.git/ | Multimedia libraries and QML modules |
| Qt Positioning | 6.8.3 | LGPL-3.0-only elected; bundled code has separate licenses | https://code.qt.io/cgit/qt/qtpositioning.git/ | Qt WebEngine runtime dependency |
| Qt SVG | 6.8.3 | LGPL-3.0-only elected; bundled code has separate licenses | https://code.qt.io/cgit/qt/qtsvg.git/ | SVG support |
| Qt WebChannel | 6.8.3 | LGPL-3.0-only elected | https://code.qt.io/cgit/qt/qtwebchannel.git/ | WebEngine/QML integration |
| Qt WebEngine | 6.8.3 | LGPL-3.0-only elected for the Qt module; embedded Chromium and its dependencies use a large mixed license set | https://code.qt.io/cgit/qt/qtwebengine.git/ | Browser libraries, QML modules, helper process, resources, and locales |
| FFmpeg | 7.1 | LGPL-2.1-or-later for the observed build configuration | https://ffmpeg.org/ | `libavcodec`, `libavformat`, `libavutil`, `libswresample`, and `libswscale` shared libraries |
| ICU | 73.2 | Unicode-3.0/ICU license family, with separate data notices | https://github.com/unicode-org/icu | Unicode libraries and data from the Qt 6.8.3 runtime; the release packager excludes the host GTK theme plugin that previously pulled ICU 78.3 into the closure |
| MsQuic | 2.4.8 | MIT AND Apache-2.0 in the installed aggregate metadata | https://github.com/microsoft/msquic | QUIC shared library |
| GCC runtime libraries | GCC 16.1.1, package `16.1.1+r12+g301eb08fa2c5-1` | GPL-3.0-or-later WITH GCC-exception-3.1 | https://gcc.gnu.org/ | Copied `libstdc++`, `libgcc_s`, and `libatomic` shared libraries |

The observed FFmpeg configuration enables shared libraries and OpenSSL and does
not contain `--enable-gpl` or `--enable-nonfree`. Its complete recorded
configuration is:

```text
--disable-programs --disable-doc --disable-debug --enable-network
--disable-lzma --enable-pic --disable-vulkan --disable-v4l2-m2m
--disable-decoder=truemotion1 --enable-shared --disable-static
--enable-openssl --prefix=/usr/local/FFmpeg-n7.1
```

The exact Qt source commits confirmed by the local Qt SPDX records are:

| Qt module | Commit |
| --- | --- |
| Qt Base | `c07c2d5a527a644d36e7853d55132ae38921682f` |
| Qt Declarative | `2ec235a2235a119887683e55787b93f99dc5f12f` |
| Qt SVG | `c75099a75e81df35e7347a65e301bbf1011831a5` |
| Qt WebEngine | `b586c4eb65d8e46ab2c255e1a141676043a650da` |

Qt's shipped modules contain additional third-party code. Confirmed examples
from the exact Qt 6.8.3 SPDX records include HarfBuzz 10.4.0 (MIT),
libjpeg-turbo 3.1.0 (IJG and BSD-3-Clause notices), libpng 1.6.47 (libpng
license family), PCRE2 10.45 (BSD-3-Clause), double-conversion 3.3.0
(BSD-3-Clause), md4c 0.5.2 (MIT), SQLite 3.49.1 (public-domain dedication),
Unicode data (Unicode-3.0), Eigen 3.4.0 (MPL-2.0 and BSD-3-Clause components),
pffft (BSD-3-Clause), Poly2Tri (BSD-3-Clause), Clipper (BSL-1.0), Clip2Tri
(MIT), and xsvg (HPND-sell-variant). This paragraph is not the complete Qt or
Chromium credits file; the exact complete upstream corpus is still required.

### Embedded Client Assets Requiring Provenance

Eight provider-logo SVGs appear to originate from Lobe Icons
(https://github.com/lobehub/lobe-icons), which is MIT-licensed, but the exact
source paths, upstream commit, and file hashes are not recorded in the current
repository. Provider names and logos can also be subject to trademark policies
that are separate from copyright licensing.

Embedded PNG/ICO assets including `head_1.png`, `ai_icon.png`,
`modelive2d.png`, and the application icon do not currently have a verifiable
project-ownership or third-party source/license record. Metadata such as
"Made with Google AI" does not grant redistribution rights. These assets are
not assigned a guessed license in this notice; their provenance must be proven
or they must be replaced before public client distribution.

Native Live2D SDK code and ignored local Live2D model/voice assets are disabled
by the Linux release preset and were not present in the inspected release
binary. They are therefore not listed as distributed components.

## C++ Backend Distribution

The C++ release bundles contain statically linked libraries plus copied MsQuic
and GCC runtime shared libraries. The inventory below is based on the actual
link commands for all 15 service targets, not merely installed packages.

| Component | Confirmed version | License | Upstream / note |
| --- | --- | --- | --- |
| Abseil | 20250814.1 | Apache-2.0 | https://github.com/abseil/abseil-cpp |
| aws-c-auth | 0.9.5 | Apache-2.0 | https://github.com/awslabs/aws-c-auth |
| aws-c-cal | 0.9.13 | Apache-2.0 | https://github.com/awslabs/aws-c-cal |
| aws-c-common | 0.12.6 | Apache-2.0 | https://github.com/awslabs/aws-c-common |
| aws-c-compression | 0.3.2 | Apache-2.0 | https://github.com/awslabs/aws-c-compression |
| aws-c-event-stream | 0.5.9 | Apache-2.0 | https://github.com/awslabs/aws-c-event-stream |
| aws-c-http | 0.10.9 | Apache-2.0 | https://github.com/awslabs/aws-c-http |
| aws-c-io | 0.26.0 | Apache-2.0 | https://github.com/awslabs/aws-c-io |
| aws-c-mqtt | 0.13.3 | Apache-2.0 | https://github.com/awslabs/aws-c-mqtt |
| aws-c-s3 | 0.11.4 | Apache-2.0 | https://github.com/awslabs/aws-c-s3 |
| aws-c-sdkutils | 0.2.4 | Apache-2.0 | https://github.com/awslabs/aws-c-sdkutils |
| aws-checksums | 0.2.8 | Apache-2.0 | https://github.com/awslabs/aws-checksums |
| AWS CRT C++ | 0.37.0 | Apache-2.0 | https://github.com/awslabs/aws-crt-cpp |
| AWS SDK for C++ | 1.11.724 | Apache-2.0 | https://github.com/aws/aws-sdk-cpp; core and S3 are linked |
| Boost | 1.90.0 | BSL-1.0 | https://www.boost.org/; Filesystem and header components including Asio/Beast |
| c-ares | 1.34.6 | MIT-CMU | https://c-ares.org/ |
| curl | 8.18.0 | BSD-3-Clause AND ISC AND curl license in the installed aggregate metadata | https://curl.se/; the verbatim aggregate notice must be retained |
| fmt | 12.1.0 | MIT | https://github.com/fmtlib/fmt |
| Glaze | 7.8.2 | MIT | https://github.com/stephenberry/glaze; header-only code is compiled into binaries |
| gRPC, gpr, upb, address_sorting | 1.71.0 | Apache-2.0 | https://github.com/grpc/grpc |
| hiredis | 1.3.0 | BSD-3-Clause | https://github.com/redis/hiredis |
| libbson | 1.30.6 | Apache-2.0 plus embedded permissive notices | https://github.com/mongodb/mongo-c-driver; retain its complete aggregate notice, including uthash, common-b64, MIT, zlib, and utf8proc material |
| PostgreSQL libpq | 16.9 | PostgreSQL License | https://www.postgresql.org/ |
| rabbitmq-c | 0.15.0 | MIT | https://github.com/alanxz/rabbitmq-c |
| librdkafka | 2.12.1 | BSD-2-Clause plus embedded MIT, Zlib, Apache-2.0, BSD, ISC, and public-domain notices | https://github.com/confluentinc/librdkafka; retain the complete aggregate notice |
| libsodium | 1.0.21 | ISC | https://github.com/jedisct1/libsodium |
| LZ4 | 1.10.0 | BSD-2-Clause | https://github.com/lz4/lz4 |
| MongoDB C Driver | 1.30.6 | Apache-2.0 plus embedded third-party notices | https://github.com/mongodb/mongo-c-driver; retain the complete aggregate notice |
| MsQuic | 2.4.8 | MIT AND Apache-2.0 in the installed aggregate metadata | https://github.com/microsoft/msquic |
| OpenSSL | 3.6.0 | Apache-2.0 | https://www.openssl.org/ |
| Protocol Buffers | 5.29.5 | BSD-3-Clause | https://github.com/protocolbuffers/protobuf |
| RE2 | 2025-11-05 | BSD-3-Clause | https://github.com/google/re2 |
| s2n-tls | 1.6.4 | Apache-2.0 | https://github.com/aws/s2n-tls |
| spdlog | 1.17.0 | MIT | https://github.com/gabime/spdlog; header/static code is compiled into binaries |
| utf8-range | 5.29.5 | MIT | https://github.com/protocolbuffers/utf8_range |
| utf8proc | 2.11.3 | MIT | https://github.com/JuliaStrings/utf8proc |
| zlib | 1.3.1 | Zlib | https://zlib.net/ |
| GCC runtime libraries | GCC 16.1.1, package `16.1.1+r12+g301eb08fa2c5-1` | GPL-3.0-or-later WITH GCC-exception-3.1 | https://gcc.gnu.org/; copied into the service bundles |

Glibc, `libm`, `libresolv`, and other host libraries excluded by the backend
packager remain system-provided and are not copied into the service bundles.

GoogleTest 1.17.0 and Google Benchmark 1.9.4 are installed development
dependencies, but release builds use `BUILD_TESTS=OFF` and neither appears in
the inspected service link commands. nghttp2 1.68.0 is installed but disabled
with `MEMOCHAT_HAVE_NGHTTP2=0`; libwebsockets/WebTransport is also disabled.
These are not represented as distributed runtime components.

## MemoChat Service Container Images

The release build creates 15 MemoChat images: `AIGatewayServer`, `AIServer`,
`AccountServer`, `CallGatewayServer`, `ChatDeliveryWorker`,
`ChatMessageService`, `ChatRelationQueryService`,
`ChatRelationServiceWorker`, `ChatServer`, `LoginServer`,
`MediaGatewayServer`, `MomentsGatewayServer`, `R18GatewayServer`,
`RegisterServer`, and `VarifyServer`.

All 15 currently use the immutable Ubuntu 24.04 base reference:

```text
ubuntu@sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90
```

Before `apt-get update`, the Dockerfile rewrites the Ubuntu sources to the fixed
`20260727T000000Z` snapshot at `snapshot.ubuntu.com`, limits suites to `noble`
and `noble-updates`, and limits components to `main` and `universe`. The minimal
base does not initially contain a CA bundle, so a bootstrap stage fetches
`ca-certificates_20260601~24.04.1_all.deb` from that same snapshot with required
SHA-256 `6bac2a01979e210d9eac1d4d56747ec709ea60654744d66705dc3c36e7629e50`,
extracts only its Mozilla certificate data, and copies the generated bundle into
the final stage before apt uses HTTPS. The final stage then installs the package
normally and verifies the installed version through `dpkg-query`.

Every service image unconditionally contains the exact added package versions
`ca-certificates=20260601~24.04.1`,
`libturbojpeg=1:2.1.5-2ubuntu2`, and
`libwebp7=1.3.2-0.4build3`; the Docker build fails if any resolved version
differs. The image labels record the base digest, snapshot, CA-bootstrap digest,
and this version tuple for later audit. Ubuntu base packages still have mixed
licenses; libjpeg-turbo uses its upstream composite BSD/IJG/Zlib notice set,
and libwebp uses BSD-3-Clause upstream terms. Exact package versions are now
fixed and machine-checked, but their authoritative Debian/Ubuntu copyright files
and the snapshot-resolved dependency closure must still be extracted from every
final image and attached with its SBOM before release.

The backend components listed above are also present in the images through the
copied service bundle. The complete image inventory is therefore the union of
the service bundle SBOM and the final Ubuntu package SBOM, not only this section.

## Separately Obtained Deployment Dependencies

The backend deployment kit ships Compose configuration that instructs an
operator's Docker installation to pull the following images. MemoChat does not
copy their filesystem layers into the client or backend archive and does not
republish those third-party images as MemoChat images. The immutable digest is
the confirmed release reference; where it does not expose an application
version, this notice does not guess one.

| External image | Release reference | Upstream application license/status |
| --- | --- | --- |
| Envoy | `envoyproxy/envoy@sha256:8146b97ee61a42cd216514709e4e3198af75f014974e3d9f310aef9c901fcbdf` | Apache-2.0; complete image license corpus accompanies the upstream image |
| BusyBox | `busybox@sha256:73aaf090f3d85aa34ee199857f03fa3a95c8ede2ffd4cc2cdb5b94e566b11662` | GPL-2.0-only upstream; complete source/notices accompany the upstream image |
| PostgreSQL | `postgres@sha256:78df81b1442dcc764c1104154da7162635e40cfffe67579c42a1c1b96dfc209c` | PostgreSQL License for PostgreSQL; image layers contain separately licensed packages |
| MongoDB | `mongo@sha256:adfc10cca964f99224ac4948e782c260c4c87e71958df334e8ca882ff85062ea` | SSPL-1.0 for MongoDB Server at the relevant release line; image corpus controls exact terms |
| MinIO | `minio/minio@sha256:14cea493d9a34af32f524e538b8346cf79f3321eff8e708c1e2960462bd8936e` | AGPL-3.0-only for MinIO Community Server; image corpus controls exact terms |
| Redis Stack Server | `redis/redis-stack-server@sha256:b569cb292d85bd0c85807a485ee9407dfc596ccd3686516e5d81d753d65405b7` | Mixed Redis/Redis Stack component terms; inspect the exact upstream image license corpus rather than assigning one license to the whole image |
| RabbitMQ | `rabbitmq@sha256:0b44fbcc3a4bf22d00090f1353127577dbe1fcb109c41669733a9d7ecf6c3a78` | MPL-2.0 for RabbitMQ Server; image layers contain separately licensed packages |
| Redpanda | `redpandadata/redpanda@sha256:c9983f650ad08015a781ec9974fe94e25c36e9f15be553db596afe179e5b2c3a` | BSL-1.1 for the applicable Redpanda release, with change-license terms defined upstream; verify against the exact image |
| InfluxDB | `influxdb@sha256:5455e9b8bb42dcb8aa0b8a0354b5f0758d0b4a01f595b8dc03e3f850aa5829e9` | MIT for InfluxDB OSS; image layers contain separately licensed packages |
| Grafana | `grafana/grafana@sha256:886b56d5534e54f69a8cfcb4b8928da8fc753178a7a3d20c3f9b04b660169805` | AGPL-3.0-only for Grafana OSS; image layers contain separately licensed packages |
| OpenTelemetry Collector Contrib | `otel/opentelemetry-collector-contrib@sha256:b65527791431d76d058b2813748a3f4a8912540d7b23beac2f6b4e02c872f5b7` | Apache-2.0; image layers contain separately licensed packages |
| cAdvisor | `gcr.io/cadvisor/cadvisor@sha256:3cde6faf0791ebf7b41d6f8ae7145466fed712ea6f252c935294d2608b1af388` | Apache-2.0; image layers contain separately licensed packages |
| Prometheus | `prom/prometheus@sha256:f6639335d34a77d9d9db382b92eeb7fc00934be8eae81dbc03b31cfe90411a94` | Apache-2.0; image layers contain separately licensed packages |
| Alertmanager | `prom/alertmanager@sha256:e13b6ed5cb929eeaee733479dce55e10eb3bc2e9c4586c705a4e8da41e5eacf5` | Apache-2.0; image layers contain separately licensed packages |
| Loki | `grafana/loki@sha256:e689cc634841c937de4d7ea6157f17e29cf257d6a320f1c293ab18d46cfea986` | AGPL-3.0-only for Loki; image layers contain separately licensed packages |
| Tempo | `grafana/tempo@sha256:f0200a9bff6d14eb3a4332194f7b77c37ee1a3535e7e41db024d95aab6f1b4e8` | AGPL-3.0-only for Tempo; image layers contain separately licensed packages |
| LiveKit Server | `livekit/livekit-server@sha256:2c6869d2d5ff6c9c0166f47be1c92dad6928bfecfa5e4060a6ece48db8accfa3` | Apache-2.0; image layers contain separately licensed packages |

The client HTML also requests LiveKit Client SDK 2.15.7 from jsDelivr at
runtime. That SDK is Apache-2.0 licensed at
https://github.com/livekit/client-sdk-js and is not copied into the inspected
client archive. This network dependency must be vendored and added to the
distributed inventory if offline or reproducible operation becomes a release
requirement.

## Required Materials Before Formal Release

The following work is still required. Its presence in this notice does not
mean it has been completed:

1. Ship the LGPL-3.0 and GPL-3.0 texts, Qt copyright notices, complete Qt 6.8.3
   third-party notices, exact corresponding Qt source or a legally valid source
   offer, and instructions that preserve replacement/relinking of dynamic Qt
   libraries. MemoChat terms must not prohibit reverse engineering needed to
   debug modifications to LGPL-covered components.
2. Extract the exact Chromium revision used by Qt WebEngine 6.8.3 and ship the
   complete generated Chromium/Qt WebEngine credits, licenses, and corresponding
   source/source offer. The installed binary Qt SPDX file explicitly cannot be
   treated as a complete Chromium third-party credits list.
3. Ship LGPL-2.1, attribution, exact FFmpeg 7.1 corresponding source, the build
   configuration recorded above, and dynamic relinking information.
4. Ship GPL-3.0, the GCC Runtime Library Exception 3.1, and exact corresponding
   source/source offer for the redistributed GCC 16.1.1 runtime package and its
   downstream patch set.
5. Ship the complete ICU 73.2 notices and data licenses, and keep the release
   contract that prevents a host ICU major from entering the client closure.
6. Copy the verbatim vcpkg `copyright` and upstream `NOTICE` materials for all
   linked backend dependencies. In particular, do not collapse curl, libbson,
   MongoDB C Driver, librdkafka, or MsQuic aggregate notices into a single SPDX
   label.
7. Establish ownership or exact upstream source, commit, file hash, license,
   and applicable trademark policy for every embedded SVG/PNG/ICO asset; replace
   any asset whose redistribution rights cannot be demonstrated.
8. Build all 15 final images, verify their fixed base/snapshot/package labels and
   `dpkg-query` results, generate SPDX or CycloneDX SBOMs from their exact
   digests, and attach the exact Ubuntu package copyright/license corpus for the
   three pinned runtime packages and their snapshot-resolved dependency closure.
9. Extend the release layout so every applicable artifact contains a nonempty
   legal corpus with verbatim licenses/notices and source-offer records. Current
   packaging copies `LICENSE`, this inventory, and `LEGAL-STATUS.txt`; because
   `legal/third-party` is still absent, it correctly marks local artifacts as
   incomplete rather than treating those two documents as sufficient for the
   copyleft and composite-license components above.
10. After the release source commit is final, generate the canonical v2 approval
    payload outside the repository and have the independent legal approver sign
    those exact bytes offline. Configure the exported public key PEM as
    `MEMOCHAT_LEGAL_APPROVAL_PUBLIC_KEY_PEM` and the detached signature's
    single-line base64 as `MEMOCHAT_LEGAL_APPROVAL_SIGNATURE_BASE64`. The private
    key must never enter Git, CI variables, build artifacts, or release archives.

Relevant primary licensing references include:

- Qt licensing: https://doc.qt.io/qt-6.8/licensing.html
- Qt third-party licenses: https://doc.qt.io/qt-6.8/licenses-used-in-qt.html
- Qt WebEngine licensing: https://doc.qt.io/qt-6.8/qtwebengine-licensing.html
- FFmpeg legal/compliance guidance: https://ffmpeg.org/legal.html

This inventory must be regenerated and reviewed whenever release dependencies,
toolchain versions, client assets, Qt modules, container bases, or external
image digests change.
