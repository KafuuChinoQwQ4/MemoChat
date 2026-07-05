#!/usr/bin/env node
/**
 * Browser-backed WebSocket chat smoke.
 *
 * Gate login is performed in Node to avoid CORS noise from a blank test page.
 * Chat transport checks run inside a real Playwright browser page using the
 * browser WebSocket API and the shared MemoChat binary frame format.
 */

import process from "node:process"

import {
  browserType,
  buildWebSocketUrl,
  gateLogin,
  loadPlaywright,
  selectEndpoint,
} from "./browser_chat_smoke_common.mjs"

function usage() {
  return `Usage:
  node tools/scripts/dev/browser_websocket_smoke.mjs --email A --password P [options]

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
  --insecure-tls               Ignore local TLS verification for Gate and browser.
  --ws-url <url>               Override WebSocket URL after login. Default uses chat_endpoints.
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
        args.timeoutMs = Number.parseInt(next(), 10)
        break
      case "--browser":
        args.browser = next()
        break
      case "--ws-url":
        args.wsUrl = next()
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

  if (!Number.isFinite(args.timeoutMs) || args.timeoutMs <= 0) {
    throw new Error("--timeout-ms must be a positive integer")
  }
  if (!["chromium", "firefox", "webkit"].includes(args.browser)) {
    throw new Error("--browser must be chromium, firefox, or webkit")
  }
  return args
}

function selectWebSocketEndpoint(gateResponse) {
  return selectEndpoint(gateResponse, "websocket")
}

async function runBrowserSmoke(args, clients) {
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
    return await page.evaluate(
      async ({ clients: browserClients, timeoutMs }) => {
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
          return buffer
        }

        function decodePayload(bytes) {
          if (bytes.byteLength === 0) return {}
          return JSON.parse(decoder.decode(bytes))
        }

        class BrowserChatProbe {
          constructor(config) {
            this.config = config
            this.buffer = new Uint8Array(0)
            this.frames = []
            this.waiters = []
            this.events = []
            this.ws = undefined
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
              ws.onclose = (event) => {
                this.events.push({ event: "close", code: event.code, reason: event.reason })
              }
              ws.onmessage = (event) => {
                void this.acceptMessage(event.data)
              }
            })
          }

          async acceptMessage(data) {
            let arrayBuffer
            if (data instanceof ArrayBuffer) {
              arrayBuffer = data
            } else if (data instanceof Blob) {
              arrayBuffer = await data.arrayBuffer()
            } else {
              throw new Error(`${this.config.label}: non-binary websocket message`)
            }
            this.appendBytes(new Uint8Array(arrayBuffer))
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

          send(msgId, payload) {
            if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
              throw new Error(`${this.config.label}: websocket is not open`)
            }
            this.ws.send(encodeFrame(msgId, payload))
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
            this.send(opcodes.MSG_CHAT_LOGIN, {
              protocol_version: this.config.protocolVersion,
              login_ticket: this.config.loginTicket,
              trace_id: `${this.config.label}-chat-login-${Date.now()}`,
            })
            const frame = await this.waitFor(
              [opcodes.MSG_CHAT_LOGIN_RSP],
              (candidate) => Number(candidate.payload.error) === 0 && Number(candidate.payload.uid) === this.config.uid,
              "chat_login",
            )
            return frame.payload
          }

          async heartbeat() {
            this.send(opcodes.ID_HEART_BEAT_REQ, {
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-heartbeat-${Date.now()}`,
            })
            const frame = await this.waitFor(
              [opcodes.ID_HEARTBEAT_RSP],
              (candidate) => Number(candidate.payload.error) === 0,
              "heartbeat",
            )
            return frame.payload
          }

          async relationBootstrap() {
            this.send(opcodes.ID_GET_RELATION_BOOTSTRAP_REQ, {
              uid: this.config.uid,
              fromuid: this.config.uid,
              protocol_version: this.config.protocolVersion,
              trace_id: `${this.config.label}-relation-bootstrap-${Date.now()}`,
            })
            const frame = await this.waitFor(
              [opcodes.ID_GET_RELATION_BOOTSTRAP_RSP],
              (candidate) => Number(candidate.payload.error) === 0 && Number(candidate.payload.uid) === this.config.uid,
              "relation_bootstrap",
            )
            return frame.payload
          }

          close() {
            if (this.ws && this.ws.readyState === WebSocket.OPEN) {
              this.ws.close(1000, "smoke_done")
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
            const probe = new BrowserChatProbe(client)
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
              wsUrl: probe.config.wsUrl,
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
            const messageId = `browser-ws-smoke-${Date.now()}`
            const content = `browser websocket smoke ${messageId}`
            sender.send(opcodes.ID_TEXT_CHAT_MSG_REQ, {
              fromuid: sender.config.uid,
              touid: receiver.config.uid,
              text_array: [{ msgid: messageId, content, created_at: Date.now() }],
              protocol_version: sender.config.protocolVersion,
              trace_id: `${sender.config.label}-private-${messageId}`,
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
            receiver.send(opcodes.ID_PRIVATE_HISTORY_REQ, {
              fromuid: receiver.config.uid,
              peer_uid: sender.config.uid,
              before_ts: 0,
              limit: 20,
              protocol_version: receiver.config.protocolVersion,
              trace_id: `${receiver.config.label}-history-${messageId}`,
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
      { clients, timeoutMs: args.timeoutMs },
    )
  } finally {
    await browser.close()
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
  const primaryEndpoint = selectWebSocketEndpoint(primaryLogin)
  if (!primaryEndpoint && !args.wsUrl) {
    throw new Error(`Gate login did not advertise a websocket chat endpoint: ${JSON.stringify(primaryLogin)}`)
  }

  const clients = [
    {
      label: "primary",
      uid: Number(primaryLogin.uid),
      protocolVersion: Number(primaryLogin.protocol_version || 3),
      loginTicket: String(primaryLogin.login_ticket),
      wsUrl: args.wsUrl || buildWebSocketUrl(primaryEndpoint),
      serverName: primaryEndpoint?.server_name || "",
    },
  ]

  if (args.peerEmail && args.peerPassword) {
    const peerLogin = await gateLogin(args, args.peerEmail, args.peerPassword)
    const peerEndpoint = selectWebSocketEndpoint(peerLogin)
    if (!peerEndpoint && !args.wsUrl) {
      throw new Error(`Peer Gate login did not advertise a websocket chat endpoint: ${JSON.stringify(peerLogin)}`)
    }
    clients.push({
      label: "peer",
      uid: Number(peerLogin.uid),
      protocolVersion: Number(peerLogin.protocol_version || 3),
      loginTicket: String(peerLogin.login_ticket),
      wsUrl: args.wsUrl || buildWebSocketUrl(peerEndpoint),
      serverName: peerEndpoint?.server_name || "",
    })
  }

  const startedAt = Date.now()
  const checks = await runBrowserSmoke(args, clients)
  const summary = {
    status: "PASS",
    elapsedMs: Date.now() - startedAt,
    gateUrl: args.gateUrl,
    browser: args.browser,
    clients: clients.map(({ label, uid, wsUrl, serverName }) => ({ label, uid, wsUrl, serverName })),
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
