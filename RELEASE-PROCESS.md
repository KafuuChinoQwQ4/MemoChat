# MemoChat Release Process

## Current Status

### ✅ Completed (Branch: chore/release-hardening)

**Legal & Compliance Gate**
- Complete third-party distribution corpus with 8 OSS license scopes (MIT, Apache-2.0, BSD-3-Clause, etc.)
- All legal verification checks passing: project license, third-party inventory, distribution corpus, source binding
- Legal review ID: `memo-legal-review-2026-08-13`
- Source snapshot bound to commit: `6ba668d4`

**CI/CD Infrastructure**
- Complete GitHub Actions workflow in `.github/workflows/ci.yml`
- Secret scanning with Gitleaks (current tree + full history on version tags)
- Release contract tests (237 tests covering security, integration, client, backend)
- Automated Linux client packaging with portable dependency bundling
- Automated backend service packaging (15 microservices)
- Container image publishing to GitHub Container Registry (ghcr.io)
- Syft + Grype security auditing (fails on High/Critical vulnerabilities)
- GitHub attestations for build provenance
- Immutable release artifact publishing

**Build & Packaging**
- Linux client build toolchain verified (GCC 16, vcpkg, Qt 6)
- Backend service build toolchain verified (GCC 16, vcpkg, C++23)
- 15 backend Docker images with deterministic builds
- Complete dependency SBOMs (SPDX format) for all services
- Legal status metadata embedded in every release artifact

### ⏳ What Happens on `git push origin v0.1.0`

When you push a semantic version tag (`vX.Y.Z`), the CI pipeline automatically:

1. **Gate Checks** (blocks the release if any fail)
   - Secret scan: full git history + all refs including `refs/pull/*`
   - Release contracts: 237 tests must pass
   - Legal verification: requires complete corpus bound to source snapshot
   - Version tag provenance: tag must be annotated, on `main`, semantic format

2. **Build Artifacts**
   - Linux client: `MemoChatQml-{sha}-linux-x86_64.tar.gz` (portable, no system deps)
   - Backend bundle: `MemoChat-backend-{sha}-linux-x86_64.tar.gz` (15 services + deployment kit)
   - All artifacts include SHA256 checksums

3. **Publish Container Images**
   - Build 15 backend service images from release bundles
   - Push to `ghcr.io/{owner}/memochat/{service}:sha-{commit}`
   - Push version aliases: `ghcr.io/{owner}/memochat/{service}:v0.1.0`
   - Embed OCI labels: commit SHA, bundle SHA256, vcpkg SBOM SHA256, legal status SHA256
   - Images are **immutable**: digest-pinned, provenance-attested

4. **Security Audit**
   - Syft: generate runtime SBOMs for all 15 images
   - Grype: CVE scan with current database
   - **Fails the build** if any High or Critical vulnerabilities found
   - Audit evidence archived with checksums: `backend-image-audit-{sha}.tar.gz`

5. **Create GitHub Release**
   - Release title: `v0.1.0`
   - Attached assets:
     - `MemoChatQml-{sha}-linux-x86_64.tar.gz` + `.sha256`
     - `MemoChat-backend-{sha}-linux-x86_64.tar.gz` + `.sha256`
     - `backend-image-audit-{sha}.tar.gz` + `.sha256`
     - `manifest.json` (structured metadata with all digests and image references)
   - GitHub build provenance attestations
   - Auto-generated release notes from commits

**Release Artifacts Structure:**
```
MemoChatQml-{sha}-linux-x86_64.tar.gz
├── bin/MemoChatQml                    # Standalone executable
├── lib/*.so.*                         # Bundled dependencies (Qt6, vcpkg libs)
├── config.ini                         # Default configuration
├── legal/
│   ├── LICENSE                        # MIT
│   ├── THIRD-PARTY-NOTICES.txt       # All dependency licenses
│   └── LEGAL-STATUS.txt              # Machine-readable release metadata
└── MANIFEST.txt                       # Checksums and build info

MemoChat-backend-{sha}-linux-x86_64.tar.gz
├── backend/
│   ├── AIServer/                      # 15 service bundles
│   ├── ChatServer/
│   ├── ...
│   └── VarifyServer/
│       ├── bin/VarifyServer           # Stripped binary
│       ├── lib/*.so.*                 # Bundled deps
│       ├── MANIFEST.txt
│       ├── SHA256SUMS
│       ├── sbom/vcpkg-build-dependencies.spdx.json
│       └── legal/LEGAL-STATUS.txt
└── deployment/
    ├── docker-compose.yml             # Production compose file
    ├── .env.example                   # Config template
    ├── Dockerfile references          # Image tags for this release
    └── legal/LEGAL-STATUS.txt

backend-image-audit-{sha}.tar.gz
├── BACKEND_IMAGES.json                # Image manifest with digests
├── SHA256SUMS                         # Evidence integrity
├── {service-1}.runtime-sbom.spdx.json # Syft container SBOM
├── {service-1}.vulnerabilities.json   # Grype CVE report
├── ...                                # 15 services × 2 files
└── AUDIT_COMPLETE                     # Completion marker
```

### 🚧 Manual Steps Required Before Release

**These blockers require human action outside the CI pipeline:**

1. **Credential Rotation** ⚠️
   - Revoke old R18 plaintext credentials (currently ignored via .gitignore)
   - Rotate external account passwords still in git history
   - Revoke leaked cookies, tokens, SMTP credentials from history
   - Update credential documentation

2. **Git History Sanitization** ⚠️
   - Current status: Gitleaks still detects sensitive patterns in history
   - GitHub-managed `refs/pull/2/head` still contains old commits
   - Required: `git filter-repo` or BFG to rewrite all refs
   - After rewrite: force-push cleaned main branch
   - Note: This **breaks existing clones** and changes all commit SHAs

3. **RC Acceptance Testing** ⏳
   - Fresh Ubuntu 24.04 VM or bare-metal test
   - Registration flow end-to-end
   - Login and session persistence
   - Chat message send/receive
   - File upload/download
   - Audio/video calls (LiveKit integration)
   - R18 profile features
   - Container restart persistence (database state)
   - Check: no High/Critical CVEs in `backend-image-audit-*.tar.gz`

4. **Version Tag Creation** 🏁
   - After PR merge to main: `git checkout main && git pull`
   - Create annotated tag: `git tag -a v0.1.0 -m "Release v0.1.0"`
   - Push tag: `git push origin v0.1.0`
   - CI automatically runs the full release pipeline (steps 1-5 above)
   - Monitor Actions: https://github.com/KafuuChinoQwQ4/MemoChat/actions

---

## How to Deploy the Release

### Option 1: Container Images (Recommended)

Pull versioned images from GHCR:
```bash
# All services use the same version tag
docker pull ghcr.io/kafuuchinoqwq4/memochat/chat-server:v0.1.0
docker pull ghcr.io/kafuuchinoqwq4/memochat/ai-server:v0.1.0
# ... (15 total services)

# Or use the deployment kit from the release archive:
tar -xzf MemoChat-backend-*-linux-x86_64.tar.gz
cd deployment
docker-compose up -d
```

**Immutability guarantee:** Version tags (`v0.1.0`) are aliases to digest-pinned images. The digest never changes after first publish.

### Option 2: Standalone Binaries

Extract and run service bundles directly:
```bash
tar -xzf MemoChat-backend-*-linux-x86_64.tar.gz
cd backend/ChatServer
./bin/ChatServer --config /path/to/config.ini
```

Each service bundle is self-contained with all dependencies.

### Option 3: Linux Client

```bash
tar -xzf MemoChatQml-*-linux-x86_64.tar.gz
cd MemoChatQml-*
./bin/MemoChatQml
```

No system dependencies required (Qt6 and vcpkg libs bundled).

---

## Verification Commands

**Verify release artifact integrity:**
```bash
# Client
sha256sum --check MemoChatQml-*.tar.gz.sha256

# Backend
sha256sum --check MemoChat-backend-*.tar.gz.sha256

# Audit evidence
sha256sum --check backend-image-audit-*.tar.gz.sha256
```

**Verify container image provenance:**
```bash
# Check image digest matches release manifest
docker buildx imagetools inspect ghcr.io/kafuuchinoqwq4/memochat/chat-server:v0.1.0

# Verify OCI labels
docker inspect ghcr.io/kafuuchinoqwq4/memochat/chat-server:v0.1.0 | jq '.[0].Config.Labels'
# Expected labels:
#   org.opencontainers.image.revision = {git-sha}
#   io.memochat.service.target = ChatServer
#   io.memochat.bundle.sha256 = {bundle-hash}
#   io.memochat.vcpkg.sbom.sha256 = {sbom-hash}
#   io.memochat.legal.status.sha256 = {legal-hash}
```

**Verify GitHub attestations:**
```bash
gh attestation verify MemoChatQml-*.tar.gz \
  --owner KafuuChinoQwQ4
```

---

## CI/CD Pipeline Jobs

| Job | Trigger | Purpose | Blocks Release |
|-----|---------|---------|----------------|
| `secret-scan` | All pushes/PRs | Gitleaks on current tree + new commits | ✅ Yes |
| `release-contracts` | All pushes/PRs | 237 security/integration/client/backend tests | ✅ Yes |
| `ai-agent-regression` | All pushes/PRs | AI orchestrator offline tests | ✅ Yes |
| `build-linux-client` | Push to main/develop/tags | Package portable Linux client | ✅ Yes (version tags) |
| `build-linux-backend` | Push to main/develop/tags | Build 15 service bundles + deployment kit | ✅ Yes (version tags) |
| `publish-backend-images` | Version tags only | Push images to GHCR, verify bindings | ✅ Yes |
| `audit-backend-images` | Version tags only | Syft+Grype CVE scan, fail on High/Critical | ✅ Yes |
| `attest-release-artifacts` | Version tags only | GitHub build provenance | ✅ Yes |
| `release-metadata` | Version tags only | Assemble manifest.json with all digests | ✅ Yes |
| `release-version-preflight` | Version tags only | Reject if release already exists | ✅ Yes |
| `publish-version-image-tags` | Version tags only | Create version tag aliases (v0.1.0) | ✅ Yes |
| `publish-github-release` | Version tags only | Upload artifacts, create GitHub Release | ✅ Yes |

**Self-hosted runner requirements:**
- `build-linux-client` and `build-linux-backend` require a self-hosted runner with:
  - Tag: `[self-hosted, Linux, X64]`
  - Build environment file: `/root/.memochat-linux-env`
  - Toolchain: GCC 16, CMake, Ninja, Qt 6, vcpkg, patchelf
  - Docker (for representative image validation)

---

## Legal Compliance

Every release artifact includes:
- `LICENSE` (MIT)
- `THIRD-PARTY-NOTICES.txt` (human-readable dependency licenses)
- `legal/LEGAL-STATUS.txt` (machine-readable metadata)
- Complete third-party corpus (8 OSS licenses with full text)

The legal gate requires:
- `formal_distribution_ready=true`
- `third_party_legal_corpus=complete`
- `release_source_sha` matches the tagged commit
- Corpus review ID and SHA256 present

All backend images carry the same legal status SHA256 in OCI labels.

---

## Next Steps Summary

1. **Open PR**: https://github.com/KafuuChinoQwQ4/MemoChat/compare/main...chore/release-hardening?expand=1
2. **Manual Actions**:
   - Rotate credentials (old R18, external tokens, SMTP)
   - Sanitize git history (BFG/filter-repo, force-push main)
   - Run RC acceptance tests on Ubuntu 24.04
3. **Merge PR** to main
4. **Create version tag**: `git tag -a v0.1.0 -m "Release v0.1.0" && git push origin v0.1.0`
5. **Monitor CI**: Wait for all jobs to complete (~30-60 minutes)
6. **Verify Release**: Check https://github.com/KafuuChinoQwQ4/MemoChat/releases/tag/v0.1.0

The CI pipeline will handle everything else automatically.
