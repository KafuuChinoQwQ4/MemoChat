#!/usr/bin/env node
/**
 * Browser-backed WebTransport chat smoke.
 *
 * Gate login is performed in Node. Browser checks run in Playwright and use
 * WebTransport reliable bidirectional streams carrying the shared MemoChat
 * uint16+uint16+JSON chat frame format.
 */

import process from "node:process"

import {
  assertSupportedBrowserName,
  browserType,
  buildWebTransportUrl,
  endpointSummary,
  gateLogin,
  loadPlaywright,
  loadServerCertificateHashes,
  parsePositiveInt,
  selectEndpoint,
} from "./browser_chat_smoke_common.mjs"

function usage() {
  return `Usage:
  node tools/scripts/dev/browser_webtransport_smoke.mjs --email A --password P [options]

Required:
  --email <email>              Primary account email. Env: MEMOCHAT_WEB_SMOKE_EMAIL
  --password <password>        Primary account password. Env: MEMOCHAT_WEB_SMOKE_PASSWORD

Options:
  --peer-email <email>         Optional peer account for private send/history.
  --peer-password <password>   Optional peer password.
  --gate-url <url>             Gate URL. Default: http://127.0.0.1:80
  --timeout-ms <ms>            Per-stage timeout. Default: 10000
  --browser <name>             chromium, firefox, or webkit. Default: chromium
  --headed                     Run headed browser.
  --insecure-tls               Ignore local TLS verification for Gate and browser context.
  --wt-url <url>               Override WebTransport URL after login. Default uses chat_endpoints.
  --certificate <path>         PEM certificate used to build browser serverCertificateHashes.
  --expect-advertised          Fail if Gate does not advertise transport=webtransport.
  --executable-path <path>     Use a system browser executable.
`
}

function parseArgs(argv) {
  const args = {
    browser: "chromium",
    gateUrl: "http://127.0.0.1:80",
    timeoutMs: 10_000,
    headed: false,
    insecureTls: false,
    expectAdvertised: false,
  }

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i]
    const next = () => {
      i += 1
      if (i >= argv.length) throw new Error(`missing value for ${arg}`)
      return argv[i]
    }

    switch (arg) {
      case "--email":
        args.email = next()
        break
      case "--password":
        args.password = next()
        break
      case "--peer-email":
        args.peerEmail = next()
        break
      case "--peer-password":
        args.peerPassword = next()
        break
      case "--gate-url":
        args.gateUrl = next()
        break
      case "--timeout-ms":
        args.timeoutMs = parsePositiveInt(next(), "--timeout-ms")
        break
      case "--browser":
        args.browser = next()
        break
      case "--wt-url":
        args.wtUrl = next()
        break
      case "--certificate":
        args.certificate = next()
        break
      case "--executable-path":
        args.executablePath = next()
        break
      case "--headed":
        args.headed = true
        break
      case "--insecure-tls":
        args.insecureTls = true
        break
      case "--expect-advertised":
        args.expectAdvertised = true
        break
      case "--help":
      case "-h":
        args.help = true
        break
      default:
        throw new Error(`unknown argument: ${arg}`)
    }
  }

  args.email = args.email ?? process.env.MEMOCHAT_WEB_SMOKE_EMAIL
  args.password = args.password ?? process.env.MEMOCHAT_WEB_SMOKE_PASSWORD
  args.peerEmail = args.peerEmail ?? process.env.MEMOCHAT_WEB_SMOKE_PEER_EMAIL
  args.peerPassword = args.peerPassword ?? process.env.MEMOCHAT_WEB_SMOKE_PEER_PASSWORD

  assertSupportedBrowserName(args.browser)
  return args
}

function buildSecureContextProbeUrl(gateUrl) {
  const url = new URL(gateUrl)
  if (url.protocol !== "https:") {
    return undefined
  }
  url.pathname = "/health"
  url.search = ""
  url.hash = ""
  return url.toString()
}

async function runBrowserSmoke(args, clients, serverCertificateHashes) {
  const playwright = loadPlaywright()
  const launchOptions = { headless: !args.headed }
  if (args.executablePath) launchOptions.executablePath = args.executablePath

  let browser
  try {
    browser = await browserType(args.browser, playwright).launch(launchOptions)
  } catch (err) {
    const detail = err instanceof Error ? err.message : String(err)
    throw new Error(`Failed to launch Playwright ${args.browser}: ${detail}\nInstall with: cd apps/web && npx playwright install chromium`)
  }

  try {
    const context = await browser.newContext({ ignoreHTTPSErrors: args.insecureTls })
    const page = await context.newPage()
    const secureContextUrl = buildSecureContextProbeUrl(args.gateUrl)
    if (secureContextUrl) {
      await page.goto(secureContextUrl, { waitUntil: "domcontentloaded", timeout: args.timeoutMs })
    }
    return await page.evaluate(
      async ({ clients: browserClients, timeoutMs, certificateHashes }) => {
        if (!("WebTransport" in globalThis)) {
          throw new Error("Browser WebTransport API is unavailable")
        }

        const opcodes = {
          MSG_CHAT_LOGIN: 1005,
          MSG_CHAT_LOGIN_RSP: 1006,
          ID_TEXT_CHAT_MSG_REQ: 1017,
          ID_TEXT_CHAT_MSG_RSP: 1018,
          ID_NOTIFY_TEXT_CHAT_MSG_REQ: 1019,
          ID_HEART_BEAT_REQ: 1023,
          ID_HEARTBEAT_RSP: 1024,
          ID_PRIVATE_HISTORY_REQ: 1059,
          ID_PRIVATE_HISTORY_RSP: 1060,
          ID_GET_RELATION_BOOTSTRAP_REQ: 1092,
          ID_GET_RELATION_BOOTSTRAP_RSP: 1093,
        }
        const encoder = new TextEncoder()
        const decoder = new TextDecoder()

        function encodeFrame(msgId, payload) {
          const body = encoder.encode(JSON.stringify(payload))
          const buffer = new ArrayBuffer(4 + body.byteLength)
          const view = new DataView(buffer)
          view.setUint16(0, msgId, false)
          view.setUint16(2, body.byteLength, false)
          new Uint8Array(buffer, 4).set(body)
          return new Uint8Array(buffer)
        }

        function decodePayload(bytes) {
          if (bytes.byteLength === 0) return {}
          return JSON.parse(decoder.decode(bytes))
        }

        function transportOptions() {
          if (!certificateHashes) return undefined
          return {
            serverCertificateHashes: certificateHashes.map((hash) => ({
              algorithm: hash.algorithm,
              value: new Uint8Array(hash.value),
            })),
          }
        }

        class WebTransportProbe {
          constructor(config) {
            this.config = config
            this.buffer = new Uint8Array(0)
            this.frames = []
            this.waiters = []
            this.events = []
            this.transport = undefined
            this.reader = undefined
            this.writer = undefined
          }

          async connect() {
            const transport = new WebTransport(this.config.wtUrl, transportOptions())
            this.transport = transport
            transport.closed.then(
              () => this.events.push({ event: "closed" }),
              (err) => this.events.push({ event: "closed_error", message: String(err && err.message ? err.message : err) }),
            )

            await this.withTimeout(transport.ready, "webtransport_ready")
            this.events.push({ event: "ready", url: this.config.wtUrl })

            const stream = await this.withTimeout(
              transport.createBidirectionalStream(),
              "webtransport_create_bidirectional_stream",
            )
            this.writer = stream.writable.getWriter()
            this.reader = stream.readable.getReader()
            void this.readLoop()
          }

          withTimeout(promise, stage) {
            return new Promise((resolve, reject) => {
              const timer = setTimeout(
                () => reject(new Error(`${this.config.label}:${stage} timed out`)),
                timeoutMs,
              )
              promise.then(
                (value) => {
                  clearTimeout(timer)
                  resolve(value)
                },
                (err) => {
                  clearTimeout(timer)
                  reject(err)
                },
              )
            })
          }

          async readLoop() {
            try {
              for (;;) {
                const result = await this.reader.read()
                if (result.done) {
                  this.events.push({ event: "stream_done" })
                  return
                }
                if (!result.value) continue
                this.appendBytes(new Uint8Array(result.value))
              }
            } catch (err) {
              this.events.push({ event: "read_error", message: String(err && err.message ? err.message : err) })
            }
          }

          appendBytes(incoming) {
            const merged = new Uint8Array(this.buffer.length + incoming.length)
            merged.set(this.buffer, 0)
            merged.set(incoming, this.buffer.length)
            this.buffer = merged

            for (;;) {
              if (this.buffer.length < 4) return
              const view = new DataView(this.buffer.buffer, this.buffer.byteOffset, this.buffer.byteLength)
              const msgId = view.getUint16(0, false)
              const size = view.getUint16(2, false)
              if (this.buffer.length < 4 + size) return
              const payloadBytes = this.buffer.slice(4, 4 + size)
              this.buffer = this.buffer.slice(4 + size)
              const frame = { msgId, payload: decodePayload(payloadBytes) }
              this.frames.push(frame)
              this.events.push({ event: "frame", msgId })
              this.drainWaiters()
            }
          }

          async send(msgId, payload) {
            if (!this.writer) {
              throw new Error(`${this.config.label}: webtransport stream is not writable`)
            }
            await this.writer.write(encodeFrame(msgId, payload))
          }

          waitFor(expectedIds, predicate, stage) {
            return new Promise((resolve, reject) => {
              const waiter = {
                expectedIds,
                predicate,
                stage,
                resolve,
                reject,
                timer: undefined,
              }
              waiter.timer = setTimeout(() => {
                this.waiters = this.waiters.filter((item) => item !== waiter)
                const recent = this.frames.slice(-5)
                reject(new Error(`${this.config.label}:${stage} timed out; recent=${JSON.stringify(recent)}`))
              }, timeoutMs)
              this.waiters.push(waiter)
              this.drainWaiters()
            })
          }

          drainWaiters() {
            for (const waiter of [...this.waiters]) {
              const match = this.frames.find(
                (frame) => waiter.expectedIds.includes(frame.msgId) && waiter.predicate(frame),
              )
              if (!match) continue
              clearTimeout(waiter.timer)
              this.waiters = this.waiters.filter((item) => item !== waiter)
              waiter.resolve(match)
            }
          }

          async login() {
            await this.send(opcodes.MSG_CHAT_LOGIN, {
              protocol_version: this.config.protocolVersion,
              login_ticket: this.config.loginTicket,
              trace_id: `${this.config.label}-wt-chat-login-${Date.now()}`,
            })
            const frame = await this.waitFor(
              [opcodes.MSG_CHAT_LOGIN_RSP],
              (candidate) => Number(candidate.payload.error) === 0 && Number(candidate.payload.uid) === this.config.uid,
              "chat_login",
            )
            return frame.payload
          }

          async heartbeat() {
            await this.send(opcodes.ID_HEART_BEAT_REQ, {
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-wt-heartbeat-${Date.now()}`,
            })
            const frame = await this.waitFor(
              [opcodes.ID_HEARTBEAT_RSP],
              (candidate) => Number(candidate.payload.error) === 0,
              "heartbeat",
            )
            return frame.payload
          }

          async relationBootstrap() {
            await this.send(opcodes.ID_GET_RELATION_BOOTSTRAP_REQ, {
              uid: this.config.uid,
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-wt-relation-bootstrap-${Date.now()}`,
            })
            const frame = await this.waitFor(
              [opcodes.ID_GET_RELATION_BOOTSTRAP_RSP],
              (candidate) => Number(candidate.payload.error) === 0 && Number(candidate.payload.uid) === this.config.uid,
              "relation_bootstrap",
            )
            return frame.payload
          }

          close() {
            try {
              this.writer?.releaseLock()
            } catch {
              // The stream may already be closed.
            }
            try {
              this.reader?.releaseLock()
            } catch {
              // The stream may already be closed.
            }
            try {
              this.transport?.close({ closeCode: 0, reason: "smoke_done" })
            } catch {
              // Browser implementations can throw during shutdown races.
            }
          }
        }

        function textArrayHas(payload, msgId, content) {
          const textArray = Array.isArray(payload.text_array) ? payload.text_array : []
          return textArray.some((item) => item && item.msgid === msgId && item.content === content)
        }

        function historyHasMessage(payload, msgId, content) {
          const messages = Array.isArray(payload.messages) ? payload.messages : []
          return messages.some((item) => item && item.msgid === msgId && item.content === content)
        }

        const probes = []
        try {
          for (const client of browserClients) {
            const probe = new WebTransportProbe(client)
            await probe.connect()
            const login = await probe.login()
            const heartbeat = await probe.heartbeat()
            const relation = await probe.relationBootstrap()
            probes.push({ probe, login, heartbeat, relation })
          }

          const checks = {
            clients: probes.map(({ probe, login, heartbeat, relation }) => ({
              label: probe.config.label,
              uid: probe.config.uid,
              wtUrl: probe.config.wtUrl,
              serverName: probe.config.serverName,
              login,
              heartbeat,
              relationBootstrapKeys: Object.keys(relation).sort(),
              events: probe.events,
            })),
          }

          if (probes.length >= 2) {
            const sender = probes[0].probe
            const receiver = probes[1].probe
            const messageId = `browser-wt-smoke-${Date.now()}`
            const content = `browser webtransport smoke ${messageId}`
            await sender.send(opcodes.ID_TEXT_CHAT_MSG_REQ, {
              fromuid: sender.config.uid,
              touid: receiver.config.uid,
              text_array: [{ msgid: messageId, content, created_at: Date.now() }],
              protocol_version: sender.config.protocolVersion,
              trace_id: `${sender.config.label}-wt-private-${messageId}`,
            })
            const ack = await sender.waitFor(
              [opcodes.ID_TEXT_CHAT_MSG_RSP],
              (candidate) =>
                Number(candidate.payload.error) === 0 && textArrayHas(candidate.payload, messageId, content),
              "private_send_ack",
            )
            const notify = await receiver.waitFor(
              [opcodes.ID_NOTIFY_TEXT_CHAT_MSG_REQ],
              (candidate) =>
                Number(candidate.payload.fromuid) === sender.config.uid &&
                Number(candidate.payload.touid) === receiver.config.uid &&
                textArrayHas(candidate.payload, messageId, content),
              "private_notify",
            )
            await receiver.send(opcodes.ID_PRIVATE_HISTORY_REQ, {
              fromuid: receiver.config.uid,
              peer_uid: sender.config.uid,
              before_ts: 0,
              limit: 20,
              protocol_version: receiver.config.protocolVersion,
              trace_id: `${receiver.config.label}-wt-history-${messageId}`,
            })
            const history = await receiver.waitFor(
              [opcodes.ID_PRIVATE_HISTORY_RSP],
              (candidate) =>
                Number(candidate.payload.error) === 0 && historyHasMessage(candidate.payload, messageId, content),
              "private_history",
            )
            checks.privateMessage = {
              status: "PASS",
              messageId,
              ack: ack.payload,
              notify: notify.payload,
              historyKeys: Object.keys(history.payload).sort(),
            }
          }

          return checks
        } finally {
          for (const { probe } of probes) probe.close()
        }
      },
      { clients, timeoutMs: args.timeoutMs, certificateHashes: serverCertificateHashes },
    )
  } finally {
    await browser.close()
  }
}

function buildClient(label, login, endpoint, wtUrlOverride) {
  if (!endpoint && !wtUrlOverride) {
    throw new Error(`${label} login response did not advertise a webtransport endpoint`)
  }
  return {
    label,
    uid: Number(login.uid),
    protocolVersion: Number(login.protocol_version || 3),
    loginTicket: String(login.login_ticket),
    wtUrl: wtUrlOverride || buildWebTransportUrl(endpoint),
    serverName: endpoint?.server_name || "",
  }
}

async function main() {
  const args = parseArgs(process.argv.slice(2))
  if (args.help) {
    console.log(usage())
    return 0
  }
  if (!args.email || !args.password) {
    throw new Error("missing primary credentials; pass --email/--password or set MEMOCHAT_WEB_SMOKE_EMAIL/PASSWORD")
  }
  if ((args.peerEmail && !args.peerPassword) || (!args.peerEmail && args.peerPassword)) {
    throw new Error("peer credentials require both --peer-email and --peer-password")
  }

  const primaryLogin = await gateLogin(args, args.email, args.password)
  const primaryEndpoint = selectEndpoint(primaryLogin, "webtransport")
  if (args.expectAdvertised && !primaryEndpoint) {
    throw new Error(`Gate login did not advertise WebTransport: ${JSON.stringify(endpointSummary(primaryLogin))}`)
  }
  if (!primaryEndpoint && !args.wtUrl) {
    const summary = {
      status: "PASS_DEFERRED",
      reason: "webtransport_not_advertised",
      gateUrl: args.gateUrl,
      endpoints: endpointSummary(primaryLogin),
    }
    console.log(JSON.stringify(summary, null, 2))
    return 0
  }

  const clients = [buildClient("primary", primaryLogin, primaryEndpoint, args.wtUrl)]
  if (args.peerEmail && args.peerPassword) {
    const peerLogin = await gateLogin(args, args.peerEmail, args.peerPassword)
    const peerEndpoint = selectEndpoint(peerLogin, "webtransport")
    if (args.expectAdvertised && !peerEndpoint) {
      throw new Error(`Peer Gate login did not advertise WebTransport: ${JSON.stringify(endpointSummary(peerLogin))}`)
    }
    clients.push(buildClient("peer", peerLogin, peerEndpoint, args.wtUrl))
  }

  const serverCertificateHashes = loadServerCertificateHashes(args.certificate)
  const startedAt = Date.now()
  const checks = await runBrowserSmoke(args, clients, serverCertificateHashes)
  const summary = {
    status: "PASS",
    elapsedMs: Date.now() - startedAt,
    gateUrl: args.gateUrl,
    browser: args.browser,
    certificateHashCount: serverCertificateHashes?.length ?? 0,
    clients: clients.map(({ label, uid, wtUrl, serverName }) => ({ label, uid, wtUrl, serverName })),
    checks,
  }
  console.log(JSON.stringify(summary, null, 2))
  return 0
}

main()
  .then((code) => {
    process.exitCode = code
  })
  .catch((err) => {
    console.error(err instanceof Error ? err.message : String(err))
    process.exitCode = 1
  })
