import fs from "node:fs"
import http from "node:http"
import https from "node:https"
import { createHash, X509Certificate } from "node:crypto"
import { createRequire } from "node:module"
import path from "node:path"
import { fileURLToPath, URL } from "node:url"

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url))
export const REPO_ROOT = path.resolve(SCRIPT_DIR, "../../..")

export function postJson(urlText, payload, { timeoutMs, insecureTls }) {
  const url = new URL(urlText)
  const body = Buffer.from(JSON.stringify(payload))
  const transport = url.protocol === "https:" ? https : http
  const options = {
    method: "POST",
    hostname: url.hostname,
    port: url.port || undefined,
    path: `${url.pathname}${url.search}`,
    headers: {
      "Content-Type": "application/json",
      "Content-Length": String(body.byteLength),
    },
    timeout: timeoutMs,
  }
  if (url.protocol === "https:") {
    options.rejectUnauthorized = !insecureTls
  }

  return new Promise((resolve, reject) => {
    const req = transport.request(options, (res) => {
      const chunks = []
      res.on("data", (chunk) => chunks.push(chunk))
      res.on("end", () => {
        const text = Buffer.concat(chunks).toString("utf8")
        const statusCode = res.statusCode ?? 0
        if (statusCode < 200 || statusCode >= 300) {
          reject(new Error(`POST ${urlText} failed http=${statusCode} body=${text}`))
          return
        }
        try {
          resolve(text ? JSON.parse(text) : {})
        } catch (err) {
          const detail = err instanceof Error ? err.message : String(err)
          reject(new Error(`POST ${urlText} returned invalid JSON: ${detail}`))
        }
      })
    })
    req.on("timeout", () => {
      req.destroy(new Error(`POST ${urlText} timed out after ${timeoutMs}ms`))
    })
    req.on("error", reject)
    req.end(body)
  })
}

export async function gateLogin(args, email, password) {
  const response = await postJson(
    `${args.gateUrl.replace(/\/$/, "")}/user_login`,
    {
      email,
      passwd: password,
      client_ver: "3.0.0",
    },
    args,
  )
  if (Number(response.error) !== 0) {
    throw new Error(`Gate login failed for ${email}: ${JSON.stringify(response)}`)
  }
  if (!response.login_ticket) {
    throw new Error(`Gate login response missing login_ticket for ${email}: ${JSON.stringify(response)}`)
  }
  return response
}

export function normalizeHost(host) {
  const value = String(host || "127.0.0.1").trim() || "127.0.0.1"
  if (value === "0.0.0.0" || value === "::" || value === "::1" || value === "localhost") {
    return "127.0.0.1"
  }
  return value
}

export function selectEndpoint(gateResponse, transportName) {
  const transport = String(transportName || "").toLowerCase()
  const endpoints = Array.isArray(gateResponse.chat_endpoints) ? gateResponse.chat_endpoints : []
  return endpoints
    .filter((endpoint) => String(endpoint.transport || "").toLowerCase() === transport)
    .sort((left, right) => Number(left.priority ?? 0) - Number(right.priority ?? 0))[0]
}

export function endpointSummary(gateResponse) {
  const endpoints = Array.isArray(gateResponse.chat_endpoints) ? gateResponse.chat_endpoints : []
  return endpoints.map((endpoint) => ({
    transport: String(endpoint.transport || ""),
    host: String(endpoint.host || ""),
    port: String(endpoint.port || ""),
    path: String(endpoint.path || ""),
    tls: Boolean(endpoint.tls),
    serverName: String(endpoint.server_name || ""),
    priority: Number(endpoint.priority ?? 0),
  }))
}

export function buildWebSocketUrl(endpoint) {
  const scheme = endpoint.tls ? "wss" : "ws"
  const host = normalizeHost(endpoint.host)
  const port = String(endpoint.port || "").trim()
  const urlPath = endpoint.path || "/ws"
  if (!port) throw new Error(`websocket endpoint missing port: ${JSON.stringify(endpoint)}`)
  return `${scheme}://${host}:${port}${urlPath}`
}

export function buildWebTransportUrl(endpoint) {
  const host = normalizeHost(endpoint.host)
  const port = String(endpoint.port || "").trim()
  const urlPath = endpoint.path || "/chat"
  if (!port) throw new Error(`webtransport endpoint missing port: ${JSON.stringify(endpoint)}`)
  return `https://${host}:${port}${urlPath}`
}

export function loadPlaywright() {
  const require = createRequire(import.meta.url)
  const searchPaths = [process.cwd(), path.join(REPO_ROOT, "apps/web"), REPO_ROOT]
  let resolved
  try {
    resolved = require.resolve("playwright", { paths: searchPaths })
  } catch (err) {
    const detail = err instanceof Error ? err.message : String(err)
    throw new Error(`Playwright package not found: ${detail}\nInstall with: cd apps/web && npm install`)
  }
  return require(resolved)
}

export function browserType(name, playwright) {
  if (name === "firefox") return playwright.firefox
  if (name === "webkit") return playwright.webkit
  return playwright.chromium
}

export function loadServerCertificateHashes(certificatePath) {
  if (!certificatePath) {
    return undefined
  }
  const pem = fs.readFileSync(certificatePath)
  const cert = new X509Certificate(pem)
  const sha256 = createHash("sha256").update(cert.raw).digest()
  return [{ algorithm: "sha-256", value: Array.from(sha256) }]
}

export function assertSupportedBrowserName(name) {
  if (!["chromium", "firefox", "webkit"].includes(name)) {
    throw new Error("--browser must be chromium, firefox, or webkit")
  }
}

export function parsePositiveInt(value, flagName) {
  const parsed = Number.parseInt(value, 10)
  if (!Number.isFinite(parsed) || parsed <= 0) {
    throw new Error(`${flagName} must be a positive integer`)
  }
  return parsed
}
