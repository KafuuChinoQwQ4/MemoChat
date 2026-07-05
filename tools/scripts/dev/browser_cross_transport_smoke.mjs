#!/usr/bin/env node
/**
 * Browser-backed cross-transport chat smoke.
 *
 * User A connects via WebSocket; user B connects via WebTransport (default)
 * or WebSocket (--skip-webtransport). Both use the shared MemoChat binary
 * frame format (uint16 msg_id + uint16 len + JSON body). The smoke verifies
 * that a message sent by A reaches B and a message sent by B reaches A,
 * exercising the shared Redis-backed online-route delivery path.
 */

import process from "node:process"

import {
  assertSupportedBrowserName,
  browserType,
  buildWebSocketUrl,
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
  node tools/scripts/dev/browser_cross_transport_smoke.mjs \\
    --email A --password P --peer-email B --peer-password Q [options]

Required:
  --email <email>              User A account email (WebSocket side).
  --password <password>        User A password.
  --peer-email <email>         User B account email (WebTransport side).
  --peer-password <password>   User B password.

Options:
  --gate-url <url>             Gate URL. Default: http://127.0.0.1:80
  --timeout-ms <ms>            Per-stage timeout. Default: 10000
  --browser <name>             chromium, firefox, or webkit. Default: chromium
  --headed                     Run headed browser.
  --insecure-tls               Ignore local TLS verification.
  --certificate <path>         PEM cert for WebTransport serverCertificateHashes.
  --expect-advertised          Fail if Gate does not advertise transport=webtransport.
  --skip-webtransport          Run both users on WebSocket (envs without WT provider).
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
    skipWebtransport: false,
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
      case "--skip-webtransport":
        args.skipWebtransport = true
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

// ---------------------------------------------------------------------------
// Browser execution
// ---------------------------------------------------------------------------

async function runBrowserSmoke(args, clients, serverCertificateHashes) {
  const playwright = loadPlaywright()
  const launchOptions = { headless: !args.headed }
  if (args.executablePath) launchOptions.executablePath = args.executablePath

  let browser
  try {
    browser = await browserType(args.browser, playwright).launch(launchOptions)
  } catch (err) {
    const detail = err instanceof Error ? err.message : String(err)
    throw new Error(
      `Failed to launch Playwright ${args.browser}: ${detail}\n` +
        `Install with: cd apps/web && npx playwright install chromium`,
    )
  }

  try {
    const context = await browser.newContext({ ignoreHTTPSErrors: args.insecureTls })
    const page = await context.newPage()

    // Navigate to the Gate HTTPS origin so the page is in a secure context,
    // which is required for WebTransport.
    const gateUrl = new URL(args.gateUrl)
    if (gateUrl.protocol === "https:") {
      const probeUrl = new URL(args.gateUrl)
      probeUrl.pathname = "/health"
      probeUrl.search = ""
      probeUrl.hash = ""
      await page.goto(probeUrl.toString(), { waitUntil: "domcontentloaded", timeout: args.timeoutMs })
    }

    return await page.evaluate(
      async ({ clients: browserClients, timeoutMs, certificateHashes }) => {
        // ── shared helpers ──────────────────────────────────────────────────
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
          const buf = new ArrayBuffer(4 + body.byteLength)
          const view = new DataView(buf)
          view.setUint16(0, msgId, false)
          view.setUint16(2, body.byteLength, false)
          new Uint8Array(buf, 4).set(body)
          return buf
        }

        function encodeFrameU8(msgId, payload) {
          return new Uint8Array(encodeFrame(msgId, payload))
        }

        function decodePayload(bytes) {
          if (bytes.byteLength === 0) return {}
          return JSON.parse(decoder.decode(bytes))
        }

        function textArrayHas(payload, msgId, content) {
          const arr = Array.isArray(payload.text_array) ? payload.text_array : []
          return arr.some((item) => item && item.msgid === msgId && item.content === content)
        }

        function historyHasMessage(payload, msgId, content) {
          const messages = Array.isArray(payload.messages) ? payload.messages : []
          return messages.some((item) => item && item.msgid === msgId && item.content === content)
        }

        // ── shared frame-parsing mixin ──────────────────────────────────────
        // Used by both probe classes to parse the streaming uint16+uint16+JSON
        // frame protocol and notify waiters.
        function makeFrameMixin(probe) {
          return {
            appendBytes(incoming) {
              const merged = new Uint8Array(probe.buffer.length + incoming.length)
              merged.set(probe.buffer, 0)
              merged.set(incoming, probe.buffer.length)
              probe.buffer = merged
              for (;;) {
                if (probe.buffer.length < 4) return
                const view = new DataView(
                  probe.buffer.buffer,
                  probe.buffer.byteOffset,
                  probe.buffer.byteLength,
                )
                const msgId = view.getUint16(0, false)
                const size = view.getUint16(2, false)
                if (probe.buffer.length < 4 + size) return
                const payloadBytes = probe.buffer.slice(4, 4 + size)
                probe.buffer = probe.buffer.slice(4 + size)
                const frame = { msgId, payload: decodePayload(payloadBytes) }
                probe.frames.push(frame)
                probe.events.push({ event: "frame", msgId })
                probe.drainWaiters()
              }
            },
          }
        }

        // ── BrowserChatProbe — WebSocket ────────────────────────────────────
        class BrowserChatProbe {
          constructor(config) {
            this.config = config
            this.buffer = new Uint8Array(0)
            this.frames = []
            this.waiters = []
            this.events = []
            this.ws = undefined
            this._mixin = makeFrameMixin(this)
          }

          async connect() {
            await new Promise((resolve, reject) => {
              const timer = setTimeout(
                () => reject(new Error(`${this.config.label}: websocket open timeout`)),
                timeoutMs,
              )
              const ws = new WebSocket(this.config.wsUrl)
              this.ws = ws
              ws.binaryType = "arraybuffer"
              ws.onopen = () => {
                clearTimeout(timer)
                this.events.push({ event: "open", url: this.config.wsUrl })
                resolve()
              }
              ws.onerror = () => {
                clearTimeout(timer)
                reject(new Error(`${this.config.label}: websocket error before open`))
              }
              ws.onclose = (ev) => {
                this.events.push({ event: "close", code: ev.code, reason: ev.reason })
              }
              ws.onmessage = (ev) => {
                void this._acceptMessage(ev.data)
              }
            })
          }

          async _acceptMessage(data) {
            let ab
            if (data instanceof ArrayBuffer) {
              ab = data
            } else if (data instanceof Blob) {
              ab = await data.arrayBuffer()
            } else {
              throw new Error(`${this.config.label}: non-binary websocket message`)
            }
            this._mixin.appendBytes(new Uint8Array(ab))
          }

          send(msgId, payload) {
            if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
              throw new Error(`${this.config.label}: websocket is not open`)
            }
            this.ws.send(encodeFrame(msgId, payload))
          }

          waitFor(expectedIds, predicate, stage) {
            return new Promise((resolve, reject) => {
              const waiter = { expectedIds, predicate, stage, resolve, reject, timer: undefined }
              waiter.timer = setTimeout(() => {
                this.waiters = this.waiters.filter((w) => w !== waiter)
                const recent = this.frames.slice(-5)
                reject(
                  new Error(`${this.config.label}:${stage} timed out; recent=${JSON.stringify(recent)}`),
                )
              }, timeoutMs)
              this.waiters.push(waiter)
              this.drainWaiters()
            })
          }

          drainWaiters() {
            for (const waiter of [...this.waiters]) {
              const match = this.frames.find(
                (f) => waiter.expectedIds.includes(f.msgId) && waiter.predicate(f),
              )
              if (!match) continue
              clearTimeout(waiter.timer)
              this.waiters = this.waiters.filter((w) => w !== waiter)
              waiter.resolve(match)
            }
          }

          async login() {
            this.send(opcodes.MSG_CHAT_LOGIN, {
              protocol_version: this.config.protocolVersion,
              login_ticket: this.config.loginTicket,
              trace_id: `${this.config.label}-ws-login-${Date.now()}`,
            })
            const f = await this.waitFor(
              [opcodes.MSG_CHAT_LOGIN_RSP],
              (c) => Number(c.payload.error) === 0 && Number(c.payload.uid) === this.config.uid,
              "chat_login",
            )
            return f.payload
          }

          async heartbeat() {
            this.send(opcodes.ID_HEART_BEAT_REQ, {
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-ws-heartbeat-${Date.now()}`,
            })
            const f = await this.waitFor(
              [opcodes.ID_HEARTBEAT_RSP],
              (c) => Number(c.payload.error) === 0,
              "heartbeat",
            )
            return f.payload
          }

          async relationBootstrap() {
            this.send(opcodes.ID_GET_RELATION_BOOTSTRAP_REQ, {
              uid: this.config.uid,
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-ws-relation-${Date.now()}`,
            })
            const f = await this.waitFor(
              [opcodes.ID_GET_RELATION_BOOTSTRAP_RSP],
              (c) =>
                Number(c.payload.error) === 0 && Number(c.payload.uid) === this.config.uid,
              "relation_bootstrap",
            )
            return f.payload
          }

          close() {
            if (this.ws && this.ws.readyState === WebSocket.OPEN) {
              this.ws.close(1000, "smoke_done")
            }
          }
        }

        // ── WebTransportProbe — WebTransport ────────────────────────────────
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
            this._mixin = makeFrameMixin(this)
          }

          _transportOptions() {
            if (!certificateHashes) return undefined
            return {
              serverCertificateHashes: certificateHashes.map((h) => ({
                algorithm: h.algorithm,
                value: new Uint8Array(h.value),
              })),
            }
          }

          _withTimeout(promise, stage) {
            return new Promise((resolve, reject) => {
              const timer = setTimeout(
                () => reject(new Error(`${this.config.label}:${stage} timed out`)),
                timeoutMs,
              )
              promise.then(
                (v) => { clearTimeout(timer); resolve(v) },
                (err) => { clearTimeout(timer); reject(err) },
              )
            })
          }

          async connect() {
            if (!("WebTransport" in globalThis)) {
              throw new Error("Browser WebTransport API is unavailable")
            }
            const wt = new WebTransport(this.config.wtUrl, this._transportOptions())
            this.transport = wt
            wt.closed.then(
              () => this.events.push({ event: "closed" }),
              (err) => this.events.push({ event: "closed_error", message: String(err && err.message ? err.message : err) }),
            )
            await this._withTimeout(wt.ready, "webtransport_ready")
            this.events.push({ event: "ready", url: this.config.wtUrl })

            const stream = await this._withTimeout(
              wt.createBidirectionalStream(),
              "webtransport_create_stream",
            )
            this.writer = stream.writable.getWriter()
            this.reader = stream.readable.getReader()
            void this._readLoop()
          }

          async _readLoop() {
            try {
              for (;;) {
                const result = await this.reader.read()
                if (result.done) { this.events.push({ event: "stream_done" }); return }
                if (!result.value) continue
                this._mixin.appendBytes(new Uint8Array(result.value))
              }
            } catch (err) {
              this.events.push({ event: "read_error", message: String(err && err.message ? err.message : err) })
            }
          }

          async send(msgId, payload) {
            if (!this.writer) throw new Error(`${this.config.label}: webtransport stream is not writable`)
            await this.writer.write(encodeFrameU8(msgId, payload))
          }

          waitFor(expectedIds, predicate, stage) {
            return new Promise((resolve, reject) => {
              const waiter = { expectedIds, predicate, stage, resolve, reject, timer: undefined }
              waiter.timer = setTimeout(() => {
                this.waiters = this.waiters.filter((w) => w !== waiter)
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
                (f) => waiter.expectedIds.includes(f.msgId) && waiter.predicate(f),
              )
              if (!match) continue
              clearTimeout(waiter.timer)
              this.waiters = this.waiters.filter((w) => w !== waiter)
              waiter.resolve(match)
            }
          }

          async login() {
            await this.send(opcodes.MSG_CHAT_LOGIN, {
              protocol_version: this.config.protocolVersion,
              login_ticket: this.config.loginTicket,
              trace_id: `${this.config.label}-wt-login-${Date.now()}`,
            })
            const f = await this.waitFor(
              [opcodes.MSG_CHAT_LOGIN_RSP],
              (c) => Number(c.payload.error) === 0 && Number(c.payload.uid) === this.config.uid,
              "chat_login",
            )
            return f.payload
          }

          async heartbeat() {
            await this.send(opcodes.ID_HEART_BEAT_REQ, {
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-wt-heartbeat-${Date.now()}`,
            })
            const f = await this.waitFor(
              [opcodes.ID_HEARTBEAT_RSP],
              (c) => Number(c.payload.error) === 0,
              "heartbeat",
            )
            return f.payload
          }

          async relationBootstrap() {
            await this.send(opcodes.ID_GET_RELATION_BOOTSTRAP_REQ, {
              uid: this.config.uid,
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-wt-relation-${Date.now()}`,
            })
            const f = await this.waitFor(
              [opcodes.ID_GET_RELATION_BOOTSTRAP_RSP],
              (c) => Number(c.payload.error) === 0 && Number(c.payload.uid) === this.config.uid,
              "relation_bootstrap",
            )
            return f.payload
          }

          close() {
            try { this.writer?.releaseLock() } catch { /* stream may already be closed */ }
            try { this.reader?.releaseLock() } catch { /* stream may already be closed */ }
            try { this.transport?.close({ closeCode: 0, reason: "smoke_done" }) } catch { /* shutdown race */ }
          }
        }

        // ── factory ─────────────────────────────────────────────────────────
        function makeProbe(client) {
          if (client.transport === "websocket") return new BrowserChatProbe(client)
          if (client.transport === "webtransport") return new WebTransportProbe(client)
          throw new Error(`unknown transport: ${client.transport}`)
        }

        // ── cross-transport runner ───────────────────────────────────────────
        const probes = []
        try {
          // Connect, login, heartbeat, relation bootstrap for each client.
          for (const client of browserClients) {
            const probe = makeProbe(client)
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
              transport: probe.config.transport,
              serverName: probe.config.serverName || "",
              login,
              heartbeat,
              relationBootstrapKeys: Object.keys(relation).sort(),
              events: probe.events,
            })),
          }

          if (probes.length < 2) {
            return checks
          }

          // ── A → B ─────────────────────────────────────────────────────────
          const probeA = probes[0].probe
          const probeB = probes[1].probe

          const msgIdAtoB = `cross-smoke-atob-${Date.now()}`
          const contentAtoB = `cross transport A-to-B ${msgIdAtoB}`

          await probeA.send(opcodes.ID_TEXT_CHAT_MSG_REQ, {
            fromuid: probeA.config.uid,
            touid: probeB.config.uid,
            text_array: [{ msgid: msgIdAtoB, content: contentAtoB, created_at: Date.now() }],
            protocol_version: probeA.config.protocolVersion,
            trace_id: `${probeA.config.label}-cross-atob-${msgIdAtoB}`,
          })

          const ackAtoB = await probeA.waitFor(
            [opcodes.ID_TEXT_CHAT_MSG_RSP],
            (c) =>
              Number(c.payload.error) === 0 &&
              textArrayHas(c.payload, msgIdAtoB, contentAtoB),
            "atob_send_ack",
          )
          const notifyAtoB = await probeB.waitFor(
            [opcodes.ID_NOTIFY_TEXT_CHAT_MSG_REQ],
            (c) =>
              Number(c.payload.fromuid) === probeA.config.uid &&
              Number(c.payload.touid) === probeB.config.uid &&
              textArrayHas(c.payload, msgIdAtoB, contentAtoB),
            "atob_notify",
          )
          checks.deliveryAtoB = {
            status: "PASS",
            messageId: msgIdAtoB,
            senderTransport: probeA.config.transport,
            receiverTransport: probeB.config.transport,
            ack: ackAtoB.payload,
            notify: notifyAtoB.payload,
          }

          // ── B → A ─────────────────────────────────────────────────────────
          const msgIdBtoA = `cross-smoke-btoa-${Date.now()}`
          const contentBtoA = `cross transport B-to-A ${msgIdBtoA}`

          await probeB.send(opcodes.ID_TEXT_CHAT_MSG_REQ, {
            fromuid: probeB.config.uid,
            touid: probeA.config.uid,
            text_array: [{ msgid: msgIdBtoA, content: contentBtoA, created_at: Date.now() }],
            protocol_version: probeB.config.protocolVersion,
            trace_id: `${probeB.config.label}-cross-btoa-${msgIdBtoA}`,
          })

          const ackBtoA = await probeB.waitFor(
            [opcodes.ID_TEXT_CHAT_MSG_RSP],
            (c) =>
              Number(c.payload.error) === 0 &&
              textArrayHas(c.payload, msgIdBtoA, contentBtoA),
            "btoa_send_ack",
          )
          const notifyBtoA = await probeA.waitFor(
            [opcodes.ID_NOTIFY_TEXT_CHAT_MSG_REQ],
            (c) =>
              Number(c.payload.fromuid) === probeB.config.uid &&
              Number(c.payload.touid) === probeA.config.uid &&
              textArrayHas(c.payload, msgIdBtoA, contentBtoA),
            "btoa_notify",
          )
          checks.deliveryBtoA = {
            status: "PASS",
            messageId: msgIdBtoA,
            senderTransport: probeB.config.transport,
            receiverTransport: probeA.config.transport,
            ack: ackBtoA.payload,
            notify: notifyBtoA.payload,
          }

          // ── history for both directions ───────────────────────────────────
          await probeB.send(opcodes.ID_PRIVATE_HISTORY_REQ, {
            fromuid: probeB.config.uid,
            peer_uid: probeA.config.uid,
            before_ts: 0,
            limit: 20,
            protocol_version: probeB.config.protocolVersion,
            trace_id: `${probeB.config.label}-cross-history-${msgIdAtoB}`,
          })
          const historyAtoB = await probeB.waitFor(
            [opcodes.ID_PRIVATE_HISTORY_RSP],
            (c) =>
              Number(c.payload.error) === 0 &&
              historyHasMessage(c.payload, msgIdAtoB, contentAtoB),
            "atob_history",
          )
          checks.historyAtoBKeys = Object.keys(historyAtoB.payload).sort()

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

// ---------------------------------------------------------------------------
// helpers for building client descriptors
// ---------------------------------------------------------------------------

function buildWsClient(label, login, endpoint, wsUrlOverride) {
  if (!endpoint && !wsUrlOverride) {
    throw new Error(`${label} login response did not advertise a websocket endpoint`)
  }
  return {
    transport: "websocket",
    label,
    uid: Number(login.uid),
    protocolVersion: Number(login.protocol_version || 3),
    loginTicket: String(login.login_ticket),
    wsUrl: wsUrlOverride || buildWebSocketUrl(endpoint),
    serverName: endpoint?.server_name || "",
  }
}

function buildWtClient(label, login, endpoint, wtUrlOverride) {
  if (!endpoint && !wtUrlOverride) {
    throw new Error(`${label} login response did not advertise a webtransport endpoint`)
  }
  return {
    transport: "webtransport",
    label,
    uid: Number(login.uid),
    protocolVersion: Number(login.protocol_version || 3),
    loginTicket: String(login.login_ticket),
    wtUrl: wtUrlOverride || buildWebTransportUrl(endpoint),
    serverName: endpoint?.server_name || "",
  }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

async function main() {
  const args = parseArgs(process.argv.slice(2))
  if (args.help) {
    console.log(usage())
    return 0
  }
  if (!args.email || !args.password) {
    throw new Error(
      "missing primary credentials; pass --email/--password or set MEMOCHAT_WEB_SMOKE_EMAIL/PASSWORD",
    )
  }
  if (!args.peerEmail || !args.peerPassword) {
    throw new Error(
      "peer credentials required for cross-transport smoke; pass --peer-email/--peer-password",
    )
  }

  // User A: always WebSocket
  const loginA = await gateLogin(args, args.email, args.password)
  const wsEndpointA = selectEndpoint(loginA, "websocket")
  if (!wsEndpointA) {
    throw new Error(
      `User A Gate login did not advertise websocket endpoint: ${JSON.stringify(endpointSummary(loginA))}`,
    )
  }
  const clientA = buildWsClient("user-a-ws", loginA, wsEndpointA, undefined)

  // User B: WebTransport (default) or WebSocket (--skip-webtransport)
  const loginB = await gateLogin(args, args.peerEmail, args.peerPassword)

  let clientB
  if (args.skipWebtransport) {
    const wsEndpointB = selectEndpoint(loginB, "websocket")
    if (!wsEndpointB) {
      throw new Error(
        `User B Gate login did not advertise websocket endpoint: ${JSON.stringify(endpointSummary(loginB))}`,
      )
    }
    clientB = buildWsClient("user-b-ws", loginB, wsEndpointB, undefined)
  } else {
    const wtEndpointB = selectEndpoint(loginB, "webtransport")
    if (args.expectAdvertised && !wtEndpointB) {
      throw new Error(
        `User B Gate login did not advertise webtransport: ${JSON.stringify(endpointSummary(loginB))}`,
      )
    }
    if (!wtEndpointB) {
      // WT not advertised — fall back to WS-only cross-transport smoke and
      // mark as PASS_DEFERRED so callers can distinguish from a hard failure.
      const wsEndpointB = selectEndpoint(loginB, "websocket")
      if (!wsEndpointB) {
        throw new Error(
          `User B Gate login did not advertise any chat endpoint: ${JSON.stringify(endpointSummary(loginB))}`,
        )
      }
      const fallbackB = buildWsClient("user-b-ws-fallback", loginB, wsEndpointB, undefined)
      const serverCertificateHashes = loadServerCertificateHashes(args.certificate)
      const startedAt = Date.now()
      const checks = await runBrowserSmoke(args, [clientA, fallbackB], serverCertificateHashes)
      console.log(
        JSON.stringify(
          {
            status: "PASS_DEFERRED",
            reason: "webtransport_not_advertised_fell_back_to_ws",
            elapsedMs: Date.now() - startedAt,
            gateUrl: args.gateUrl,
            browser: args.browser,
            clientA: { label: clientA.label, uid: clientA.uid, transport: clientA.transport },
            clientB: { label: fallbackB.label, uid: fallbackB.uid, transport: fallbackB.transport },
            checks,
          },
          null,
          2,
        ),
      )
      return 0
    }
    clientB = buildWtClient("user-b-wt", loginB, wtEndpointB, undefined)
  }

  const serverCertificateHashes = loadServerCertificateHashes(args.certificate)
  const startedAt = Date.now()
  const checks = await runBrowserSmoke(args, [clientA, clientB], serverCertificateHashes)
  const summary = {
    status: "PASS",
    elapsedMs: Date.now() - startedAt,
    gateUrl: args.gateUrl,
    browser: args.browser,
    clientA: { label: clientA.label, uid: clientA.uid, transport: clientA.transport },
    clientB: { label: clientB.label, uid: clientB.uid, transport: clientB.transport },
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
