#!/usr/bin/env node
/**
 * WebTransport provider dependency preflight.
 *
 * This does not start services or install packages. It makes the optional
 * libwebsockets HTTP/3 WebTransport provider state explicit before someone
 * enables MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER.
 */

import fs from "node:fs"
import path from "node:path"
import process from "node:process"
import { fileURLToPath } from "node:url"

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url))
const REPO_ROOT = path.resolve(SCRIPT_DIR, "../../..")

function usage() {
  return `Usage:
  node tools/scripts/dev/webtransport_provider_preflight.mjs [options]

Options:
  --json                    Print machine-readable JSON.
  --strict                  Exit non-zero unless provider prerequisites are ready.
  --repo-root <path>        Repository root. Default: auto-detected.
  --vcpkg-root <path>       vcpkg root. Default: VCPKG_ROOT or /data/vcpkg/vcpkg.
  --installed-dir <path>    vcpkg installed dir. Default: VCPKG_INSTALLED_DIR or /data/vcpkg/installed-memochat-gcc16.
  --triplet <name>          vcpkg triplet. Default: VCPKG_TARGET_TRIPLET or x64-linux-release.
  --build-dir <path>        CMake build dir. Default: <repo-root>/build-linux-full-gcc16.
  --help, -h                Show this help.
`
}

function parseArgs(argv) {
  const args = {
    json: false,
    strict: false,
    repoRoot: REPO_ROOT,
    vcpkgRoot: process.env.VCPKG_ROOT || "/data/vcpkg/vcpkg",
    installedDir: process.env.VCPKG_INSTALLED_DIR || "/data/vcpkg/installed-memochat-gcc16",
    triplet: process.env.VCPKG_TARGET_TRIPLET || "x64-linux-release",
    buildDir: "",
  }
  args.buildDir = path.join(args.repoRoot, "build-linux-full-gcc16")

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i]
    const next = () => {
      i += 1
      if (i >= argv.length) throw new Error(`missing value for ${arg}`)
      return argv[i]
    }

    switch (arg) {
      case "--json":
        args.json = true
        break
      case "--strict":
        args.strict = true
        break
      case "--repo-root":
        args.repoRoot = path.resolve(next())
        if (!args.buildDir || args.buildDir === path.join(REPO_ROOT, "build-linux-full-gcc16")) {
          args.buildDir = path.join(args.repoRoot, "build-linux-full-gcc16")
        }
        break
      case "--vcpkg-root":
        args.vcpkgRoot = path.resolve(next())
        break
      case "--installed-dir":
        args.installedDir = path.resolve(next())
        break
      case "--triplet":
        args.triplet = next()
        break
      case "--build-dir":
        args.buildDir = path.resolve(next())
        break
      case "--help":
      case "-h":
        args.help = true
        break
      default:
        throw new Error(`unknown argument: ${arg}`)
    }
  }
  return args
}

function fileExists(filePath) {
  try {
    return fs.statSync(filePath).isFile()
  } catch {
    return false
  }
}

function dirExists(dirPath) {
  try {
    return fs.statSync(dirPath).isDirectory()
  } catch {
    return false
  }
}

function readTextIfExists(filePath) {
  if (!fileExists(filePath)) return ""
  return fs.readFileSync(filePath, "utf8")
}

function readJsonIfExists(filePath) {
  const text = readTextIfExists(filePath)
  if (!text) return null
  try {
    return JSON.parse(text)
  } catch {
    return null
  }
}

function cacheValue(cacheText, name) {
  const pattern = new RegExp(`^${name}:[^=]*=(.*)$`, "m")
  const match = cacheText.match(pattern)
  return match ? match[1].trim() : undefined
}

function check(id, ok, severity, detail, evidence = {}) {
  return {
    id,
    ok: Boolean(ok),
    severity,
    detail,
    evidence,
  }
}

function allTokensPresent(text, tokens) {
  return tokens.every((token) => text.includes(token))
}

function resolveOverlayPath(repoRoot, overlayPath) {
  return path.resolve(repoRoot, overlayPath)
}

function featureObject(manifest, featureName) {
  if (!manifest || typeof manifest !== "object") return null
  const features = manifest.features
  if (!features || typeof features !== "object") return null
  const feature = features[featureName]
  return feature && typeof feature === "object" ? feature : null
}

function dependencyNames(dependencies) {
  if (!Array.isArray(dependencies)) return []
  return dependencies
    .map((dependency) => {
      if (typeof dependency === "string") return dependency
      if (dependency && typeof dependency === "object" && typeof dependency.name === "string") return dependency.name
      return ""
    })
    .filter(Boolean)
}

function cmakeListItems(value) {
  if (!value) return []
  return value
    .split(";")
    .map((item) => item.trim())
    .filter(Boolean)
}

function runPreflight(args) {
  const repoRoot = args.repoRoot
  const rootManifest = path.join(repoRoot, "vcpkg.json")
  const rootManifestJson = readJsonIfExists(rootManifest)
  const vcpkgConfiguration = path.join(repoRoot, "vcpkg-configuration.json")
  const vcpkgConfigurationJson = readJsonIfExists(vcpkgConfiguration)

  const coreCmake = path.join(repoRoot, "apps/server/core/CMakeLists.txt")
  const chatCmake = path.join(repoRoot, "apps/server/core/ChatServer/CMakeLists.txt")
  const providerHeader = path.join(repoRoot, "apps/server/core/ChatServer/transport/IWebTransportProvider.hpp")
  const providerSource = path.join(repoRoot, "apps/server/core/ChatServer/transport/LibwebsocketsWebTransportProvider.cpp")
  const unavailableSource = path.join(repoRoot, "apps/server/core/ChatServer/transport/UnavailableWebTransportProvider.cpp")

  const coreCmakeText = readTextIfExists(coreCmake)
  const chatCmakeText = readTextIfExists(chatCmake)
  const providerHeaderText = readTextIfExists(providerHeader)
  const providerSourceText = readTextIfExists(providerSource)
  const unavailableSourceText = readTextIfExists(unavailableSource)

  const repoOverlayPortDir = path.join(repoRoot, "infra/ports/libwebsockets")
  const builtinPortDir = path.join(args.vcpkgRoot, "ports/libwebsockets")
  const overlayPorts = Array.isArray(vcpkgConfigurationJson?.["overlay-ports"])
    ? vcpkgConfigurationJson["overlay-ports"]
    : []
  const normalizedOverlayPorts = overlayPorts.map((overlayPath) => resolveOverlayPath(repoRoot, overlayPath))
  const repoOverlayConfigured = normalizedOverlayPorts.includes(repoOverlayPortDir)
  const vcpkgPortDir = dirExists(repoOverlayPortDir) ? repoOverlayPortDir : builtinPortDir
  const vcpkgPortSource = dirExists(repoOverlayPortDir) ? "repo-overlay" : "vcpkg-builtin"
  const vcpkgPortfile = path.join(vcpkgPortDir, "portfile.cmake")
  const h3WebTransportPatch = path.join(vcpkgPortDir, "patches/enable-h3-webtransport-settings.patch")
  const vcpkgManifest = path.join(vcpkgPortDir, "vcpkg.json")
  const vcpkgPortfileText = readTextIfExists(vcpkgPortfile)
  const h3WebTransportPatchText = readTextIfExists(h3WebTransportPatch)
  const vcpkgManifestText = readTextIfExists(vcpkgManifest)
  const webtransportFeature = featureObject(rootManifestJson, "webtransport-provider")
  const webtransportFeatureDeps = dependencyNames(webtransportFeature?.dependencies)
  const defaultFeatures = Array.isArray(rootManifestJson?.["default-features"]) ? rootManifestJson["default-features"] : []

  const installedTripletDir = path.join(args.installedDir, args.triplet)
  const lwsHeader = path.join(installedTripletDir, "include/libwebsockets.h")
  const webtransportHeader = path.join(installedTripletDir, "include/libwebsockets/lws-webtransport.h")
  const configCandidates = [
    path.join(installedTripletDir, "share/libwebsockets/libwebsockets-config.cmake"),
    path.join(installedTripletDir, "share/libwebsockets/libwebsocketsConfig.cmake"),
    path.join(installedTripletDir, "lib/cmake/libwebsockets/libwebsockets-config.cmake"),
    path.join(installedTripletDir, "lib/cmake/libwebsockets/libwebsocketsConfig.cmake"),
  ]
  const configFile = configCandidates.find(fileExists)

  const cmakeCache = path.join(args.buildDir, "CMakeCache.txt")
  const cmakeCacheText = readTextIfExists(cmakeCache)
  const providerOption = cacheValue(cmakeCacheText, "MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER")
  const cmakeManifestFeatureValue = cacheValue(cmakeCacheText, "VCPKG_MANIFEST_FEATURES") ?? ""
  const cmakeManifestFeatures = cmakeListItems(cmakeManifestFeatureValue)

  const cmakeProbeTokens = [
    "MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER",
    "find_package(libwebsockets CONFIG REQUIRED)",
    "lws-webtransport.h",
    "LWS_WITH_HTTP3",
    "LWS_ROLE_WT",
    "lws_wt_create_stream",
    "lws_wt_is_session",
    "WSI_TOKEN_HTTP_COLON_PATH",
  ]
  const providerSourceTokens = [
    "#if MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER",
    "LibwebsocketsWebTransportProvider",
    "lws_wt_create_stream",
    "lws_wt_is_session",
    "acceptStreamBytes",
  ]
  const portfileWebTransportTokens = [
    "LWS_WITH_HTTP3",
    "LWS_ROLE_WT",
    "lws-webtransport",
  ]
  const h3WebTransportPatchTokens = [
    "H2SET_ENABLE_CONNECT_PROTOCOL",
    "HTTP_STATUS_OK",
    "LWS_WRITE_HTTP_HEADERS",
    "wsi->wt.is_session = 1",
    "LWS_WT_STREAM_TYPE_BIDI",
    "lws_quic_find_wt_session",
    "wt_session_wsi ? &role_ops_wt",
    "wsi_child->parent = wt_session_wsi",
    "wsi_child->role_ops->rx_cb",
    "lws_get_quic_network_wsi",
    "rops_callback_on_writable_wt",
    ".callback_on_writable  = rops_callback_on_writable_wt",
    "/* LWS_ROPS_handle_POLLIN */\t\t\t0x01",
    "/* LWS_ROPS_tx_credit */\t\t\t0x40",
    "/* LWS_ROPS_encapsulation_parent */\t\t0x20",
    "/* LWS_ROPS_close_kill_connection */\t\t0x03",
    "lws_wsi_mux_mark_parents_needing_writeable(wsi);",
    "return lws_callback_on_writable(nwsi);",
  ]

  const checks = [
    check(
      "repo-libwebsockets-overlay-configured",
      dirExists(repoOverlayPortDir) && repoOverlayConfigured,
      "dependency",
      "The repository libwebsockets overlay port must be listed in vcpkg-configuration.json.",
      {
        configuration: path.relative(repoRoot, vcpkgConfiguration),
        overlayPort: path.relative(repoRoot, repoOverlayPortDir),
        configuredOverlayPorts: overlayPorts,
      },
    ),
    check(
      "manifest-webtransport-provider-feature",
      Boolean(webtransportFeature) && webtransportFeatureDeps.includes("libwebsockets"),
      "dependency",
      "The root vcpkg manifest must expose a non-default webtransport-provider feature depending on libwebsockets.",
      {
        manifest: path.relative(repoRoot, rootManifest),
        feature: "webtransport-provider",
        dependencies: webtransportFeatureDeps,
      },
    ),
    check(
      "manifest-webtransport-provider-nondefault",
      Boolean(webtransportFeature) && !defaultFeatures.includes("webtransport-provider"),
      "dependency",
      "The experimental WebTransport provider dependency must not be pulled into default builds.",
      {
        manifest: path.relative(repoRoot, rootManifest),
        defaultFeatures,
      },
    ),
    check(
      "chatserver-cmake-provider-gate",
      fileExists(coreCmake) && allTokensPresent(coreCmakeText, cmakeProbeTokens),
      "required",
      "ChatServer core CMake must keep the libwebsockets provider behind an explicit HTTP/3 WebTransport compile probe.",
      { path: path.relative(repoRoot, coreCmake), tokens: cmakeProbeTokens },
    ),
    check(
      "chatserver-transport-conditional-source",
      fileExists(chatCmake) &&
        chatCmakeText.includes("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER_COMPILED") &&
        chatCmakeText.includes("transport/LibwebsocketsWebTransportProvider.cpp"),
      "required",
      "ChatTransportCore must compile the libwebsockets provider only when the CMake probe has succeeded.",
      { path: path.relative(repoRoot, chatCmake) },
    ),
    check(
      "provider-interface-boundary",
      fileExists(providerHeader) &&
        providerHeaderText.includes("class IWebTransportProvider") &&
        !providerHeaderText.includes("LibwebsocketsWebTransportProvider") &&
        !providerHeaderText.includes('#include "WebTransportSession.hpp"'),
      "required",
      "The provider boundary must not expose concrete libwebsockets or WebTransport session implementation headers.",
      { path: path.relative(repoRoot, providerHeader) },
    ),
    check(
      "libwebsockets-provider-source",
      fileExists(providerSource) && allTokensPresent(providerSourceText, providerSourceTokens),
      "required",
      "The optional libwebsockets provider implementation must exist and use WebTransport stream APIs.",
      { path: path.relative(repoRoot, providerSource), tokens: providerSourceTokens },
    ),
    check(
      "unavailable-provider-default",
      fileExists(unavailableSource) && unavailableSourceText.includes("webtransport_h3_library_not_configured"),
      "required",
      "Default builds must keep an unavailable provider with a clear failure reason.",
      { path: path.relative(repoRoot, unavailableSource) },
    ),
    check(
      "vcpkg-libwebsockets-port-present",
      dirExists(vcpkgPortDir) && fileExists(vcpkgManifest) && fileExists(vcpkgPortfile),
      "dependency",
      "A vcpkg libwebsockets port is needed before a controlled provider build can be attempted.",
      { source: vcpkgPortSource, portDir: vcpkgPortDir, manifest: vcpkgManifest, portfile: vcpkgPortfile },
    ),
    check(
      "vcpkg-libwebsockets-port-h3-webtransport",
      fileExists(vcpkgPortfile) && allTokensPresent(vcpkgPortfileText, portfileWebTransportTokens),
      "dependency",
      "The selected vcpkg libwebsockets port must visibly enable HTTP/3 and WebTransport role support.",
      { source: vcpkgPortSource, portfile: vcpkgPortfile, tokens: portfileWebTransportTokens },
    ),
    check(
      "vcpkg-libwebsockets-h3-webtransport-patch",
      fileExists(h3WebTransportPatch) && allTokensPresent(h3WebTransportPatchText, h3WebTransportPatchTokens),
      "dependency",
      "The selected libwebsockets overlay patch must enable extended CONNECT, accept CONNECT with 200 headers, mark WT sessions, dispatch WT streams, and register WT role ops for receive, writable, write, and close callbacks.",
      { source: vcpkgPortSource, patch: h3WebTransportPatch, tokens: h3WebTransportPatchTokens },
    ),
    check(
      "vcpkg-libwebsockets-port-version-visible",
      fileExists(vcpkgManifest) && vcpkgManifestText.includes('"name"') && vcpkgManifestText.includes("libwebsockets"),
      "info",
      "The selected libwebsockets vcpkg manifest should be readable for auditability.",
      { source: vcpkgPortSource, manifest: vcpkgManifest },
    ),
    check(
      "installed-libwebsockets-core-header",
      fileExists(lwsHeader),
      "dependency",
      "The active vcpkg installed tree must contain libwebsockets.h.",
      { header: lwsHeader },
    ),
    check(
      "installed-libwebsockets-webtransport-header",
      fileExists(webtransportHeader),
      "dependency",
      "The active vcpkg installed tree must contain libwebsockets/lws-webtransport.h.",
      { header: webtransportHeader },
    ),
    check(
      "installed-libwebsockets-cmake-config",
      Boolean(configFile),
      "dependency",
      "The active vcpkg installed tree must contain a libwebsockets CMake config file.",
      { candidates: configCandidates, found: configFile || "" },
    ),
    check(
      "cmake-cache-provider-option",
      providerOption !== undefined,
      "info",
      "The current CMake cache should expose MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER.",
      { cache: cmakeCache, value: providerOption ?? "" },
    ),
  ]

  const requiredFailures = checks.filter((item) => item.severity === "required" && !item.ok)
  const dependencyFailures = checks.filter((item) => item.severity === "dependency" && !item.ok)
  const providerEnabled = providerOption === "ON" || providerOption === "1" || providerOption === "TRUE"
  checks.push(
    check(
      "cmake-cache-webtransport-provider-feature",
      !providerEnabled || cmakeManifestFeatures.includes("webtransport-provider"),
      "dependency",
      "Provider-enabled CMake caches must include the webtransport-provider vcpkg manifest feature so vcpkg does not remove libwebsockets during configure.",
      {
        cache: cmakeCache,
        providerOption: providerOption ?? "",
        manifestFeatures: cmakeManifestFeatures,
      },
    ),
  )
  const strictFailures = checks.filter((item) => item.severity !== "info" && !item.ok)

  let status = "PASS"
  let exitCode = 0
  if (requiredFailures.length > 0) {
    status = "FAIL"
    exitCode = 1
  } else if (dependencyFailures.length > 0) {
    status = providerEnabled ? "FAIL" : "DEFERRED"
    exitCode = providerEnabled ? 1 : 0
  }
  if (args.strict && strictFailures.length > 0) {
    status = "FAIL"
    exitCode = 1
  }

  const summary = {
    status,
    strict: args.strict,
    providerEnabled,
    repoRoot,
    vcpkgRoot: args.vcpkgRoot,
    selectedLibwebsocketsPortSource: vcpkgPortSource,
    selectedLibwebsocketsPortDir: vcpkgPortDir,
    repoOverlayPortDir,
    installedDir: args.installedDir,
    triplet: args.triplet,
    buildDir: args.buildDir,
    providerOption: providerOption ?? null,
    cmakeManifestFeatures,
    checks,
    failures: checks.filter((item) => !item.ok).map((item) => item.id),
    next: buildNextAction(status, vcpkgPortSource, dependencyFailures),
  }
  return { summary, exitCode }
}

function buildNextAction(status, vcpkgPortSource, dependencyFailures) {
  if (status === "PASS") {
    return "Provider prerequisites are present. Configure with MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER=ON and run browser WebTransport smoke before advertising."
  }

  const missingIds = new Set(dependencyFailures.map((item) => item.id))
  const onlyInstalledTreeMissing =
    missingIds.size > 0 &&
    [...missingIds].every((id) =>
      [
        "installed-libwebsockets-core-header",
        "installed-libwebsockets-webtransport-header",
        "installed-libwebsockets-cmake-config",
      ].includes(id),
    )

  if (vcpkgPortSource === "repo-overlay" && onlyInstalledTreeMissing) {
    return "Keep WebTransport hidden. Install the webtransport-provider vcpkg feature from the repo overlay, then rerun this preflight in --strict mode."
  }

  return "Keep WebTransport hidden. Add or overlay a libwebsockets build with HTTP/3 WebTransport support, then rerun this preflight in --strict mode."
}

function printHuman(summary) {
  console.log(`WebTransport provider preflight: ${summary.status}`)
  console.log(`provider option: ${summary.providerOption ?? "not in cache"}`)
  console.log(`vcpkg root: ${summary.vcpkgRoot}`)
  console.log(`selected libwebsockets port: ${summary.selectedLibwebsocketsPortSource} (${summary.selectedLibwebsocketsPortDir})`)
  console.log(`installed dir: ${summary.installedDir}`)
  console.log(`triplet: ${summary.triplet}`)
  for (const item of summary.checks) {
    const mark = item.ok ? "OK" : "MISS"
    console.log(`  [${mark}] ${item.id} (${item.severity})`)
    if (!item.ok) {
      console.log(`       ${item.detail}`)
    }
  }
  console.log(summary.next)
}

function main() {
  const args = parseArgs(process.argv.slice(2))
  if (args.help) {
    console.log(usage())
    return 0
  }
  const { summary, exitCode } = runPreflight(args)
  if (args.json) {
    console.log(JSON.stringify(summary, null, 2))
  } else {
    printHuman(summary)
  }
  return exitCode
}

try {
  process.exitCode = main()
} catch (err) {
  console.error(err instanceof Error ? err.message : String(err))
  process.exitCode = 1
}
