/**
 * postLoginBootstrap — exact login sequence per architecture plan §5.
 * 1. HTTP login → store session
 * 2. WS connect → send chat-login (1005) → wait for 1006
 * 3. Send relation, group-list, and dialog-list bootstraps
 * 4. Start heartbeat
 * Errors: 1095/1096 → refresh; 1097 → re-fetch endpoints; 1098 → hard fail.
 */
import { useSessionStore } from "@/core/session/sessionStore"
import type { ChatEndpointInfo } from "@/core/session/sessionTypes"
import { useEntityStore } from "@/core/entities/entityStore"
import { normalizePublicUserId } from "@/core/entities/displayIds"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { ErrorCodes } from "@/core/network/opcodes/errorCodes"
import { runtimeConfig } from "@/core/config/runtimeConfig"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { logger } from "@/core/common/logger"
import type { ServerInfo } from "@/core/network/transport/transport.types"
import { applyDialogListPayload, applyGroupListPayload } from "@/app/dispatch/chatListPayloads"
import type { ApplyEntry, Friend } from "@/core/entities/entityTypes"
import { ENDPOINTS } from "@/core/config/endpoints"

export interface LoginCredentials {
  email: string
  password: string
}

/** Mirrors AuthLoginResponseDto — fields are at the top level, not nested under "user" */
export interface LoginResponse {
  error: number
  uid: number
  user_id: string
  access_token: string
  login_ticket: string
  ticket_expire_ms: number
  protocol_version: number
  preferred_transport?: string
  fallback_transport?: string
  user_profile: {
    uid: number
    user_id: string
    name: string
    nick: string
    icon: string
    desc: string
    email: string
    sex: number
  }
  chat_endpoints: Array<{
    transport: string
    host: string
    port: string
    path?: string
    tls?: boolean
    server_name: string
    priority: number
  }>
}

interface BrowserChatEndpointSelection {
  primary: LoginResponse["chat_endpoints"][number]
  websocketFallback?: LoginResponse["chat_endpoints"][number]
}

export function selectBrowserChatEndpoint(
  endpoints: LoginResponse["chat_endpoints"],
  preferWebTransport: boolean,
): BrowserChatEndpointSelection | null {
  const byPriority = (left: LoginResponse["chat_endpoints"][number],
                      right: LoginResponse["chat_endpoints"][number]) =>
    left.priority - right.priority
  const websocket = endpoints
    .filter((ep) => ep.transport === "websocket")
    .sort(byPriority)[0]
  const webtransport = endpoints
    .filter((ep) => ep.transport === "webtransport")
    .sort(byPriority)[0]

  if (preferWebTransport && webtransport) {
    const selection: BrowserChatEndpointSelection = { primary: webtransport }
    if (websocket) selection.websocketFallback = websocket
    return selection
  }
  if (websocket) {
    return { primary: websocket }
  }
  return null
}

/**
 * Resolve WebSocket protocol independently of the server-supplied tls flag.
 * A compromised or misconfigured server could set tls=false to silently
 * downgrade clients to plaintext. We ignore that hint for non-local hosts and
 * always use wss://, preventing credential theft over the wire (H-21).
 */
function resolveWsProtocol(host: string, serverTls: boolean): 'ws' | 'wss' {
  // Always use wss in production (non-localhost)
  const isLocalhost =
    host === 'localhost' ||
    host === '127.0.0.1' ||
    host.startsWith('192.168.') ||
    host.endsWith('.local')
  if (!isLocalhost) return 'wss'
  // For localhost, respect server hint
  return serverTls ? 'wss' : 'ws'
}

export function buildEndpointUrl(endpoint: LoginResponse["chat_endpoints"][number]): string {
  const path = endpoint.path ?? (endpoint.transport === "webtransport" ? "/chat" : "/ws")
  if (endpoint.transport === "webtransport") {
    return `https://${endpoint.host}:${endpoint.port}${path}`
  }
  const scheme = resolveWsProtocol(endpoint.host, endpoint.tls ?? false)
  return `${scheme}://${endpoint.host}:${endpoint.port}${path}`
}

export async function postLoginBootstrap(creds: LoginCredentials): Promise<void> {
  const gateway = getGateway()
  const session = useSessionStore.getState()

  // Step 1: HTTP login
  const res = await gateway.http.post<LoginResponse>(ENDPOINTS.login, {
    email: creds.email,
    passwd: creds.password,
    client_ver: "3.0.0",
  }, {
    auth: false,
    headers: { "X-MemoChat-Client": "web" },
  })

  const successCode: number = ErrorCodes.SUCCESS
  if (res.error !== successCode) {
    throw new Error(`Login failed: error code ${res.error}`)
  }

  session.setLogin({
    uid: res.uid,
    token: res.access_token,
    loginTicket: res.login_ticket,
    ticketExpireMs: res.ticket_expire_ms,
    protocolVersion: res.protocol_version,
    chatEndpoints: res.chat_endpoints.map((ep) => {
      const endpoint: ChatEndpointInfo = {
        transport: ep.transport,
        host: ep.host,
        port: ep.port,
        serverName: ep.server_name,
        priority: ep.priority,
      }
      if (ep.path !== undefined) endpoint.path = ep.path
      if (ep.tls !== undefined) endpoint.tls = ep.tls
      return endpoint
    }),
    profile: {
      uid: res.user_profile.uid,
      name: res.user_profile.name,
      email: res.user_profile.email,
      icon: res.user_profile.icon,
      userId: res.user_profile.user_id,
    },
  })

  // Step 2: Connect WS transport
  session.setConnState("connecting")
  await connectAndChatLogin(gateway, res)

  // Step 3: Entity bootstrap
  await sendRelationBootstrap(gateway, res.uid)
  await Promise.all([
    sendGroupListBootstrap(gateway, res.uid),
    sendDialogListBootstrap(gateway, res.uid),
  ])

  logger.app.info("Bootstrap complete for uid", res.uid)
}

/**
 * Restore a previously persisted web session without password.
 * Uses /auth/refresh to rotate refresh_token and re-issue access_token + login_ticket,
 * then reconnects chat transport and re-runs list bootstraps.
 */
export async function restoreSessionFromRefresh(): Promise<void> {
  const gateway = getGateway()
  const session = useSessionStore.getState()

  const res = await gateway.http.post<LoginResponse>(ENDPOINTS.authRefresh, {
    client_ver: "3.0.0",
  }, {
    auth: false,
    headers: { "X-MemoChat-Client": "web" },
  })

  const successCode: number = ErrorCodes.SUCCESS
  if (res.error !== successCode) {
    throw new Error(`Session restore failed: error code ${res.error}`)
  }
  if (!res.access_token || !res.login_ticket) {
    throw new Error("Session restore missing access credentials")
  }

  session.setLogin({
    uid: res.uid,
    token: res.access_token,
    loginTicket: res.login_ticket,
    ticketExpireMs: res.ticket_expire_ms,
    protocolVersion: res.protocol_version,
    chatEndpoints: (res.chat_endpoints ?? []).map((ep) => {
      const endpoint: ChatEndpointInfo = {
        transport: ep.transport,
        host: ep.host,
        port: ep.port,
        serverName: ep.server_name,
        priority: ep.priority,
      }
      if (ep.path !== undefined) endpoint.path = ep.path
      if (ep.tls !== undefined) endpoint.tls = ep.tls
      return endpoint
    }),
    profile: {
      uid: res.user_profile.uid,
      name: res.user_profile.name,
      email: res.user_profile.email,
      icon: res.user_profile.icon,
      userId: res.user_profile.user_id,
    },
  })

  session.setConnState("connecting")
  await connectAndChatLogin(gateway, res)
  await sendRelationBootstrap(gateway, res.uid)
  await Promise.all([
    sendGroupListBootstrap(gateway, res.uid),
    sendDialogListBootstrap(gateway, res.uid),
  ])
  logger.app.info("Session restore complete for uid", res.uid)
}

async function connectAndChatLogin(
  gateway: ReturnType<typeof getGateway>,
  res: LoginResponse,
): Promise<void> {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => reject(new Error("Chat login timeout")), 10_000)

    gateway.chatTransport.onReady = () => {
      clearTimeout(timeout)
      useSessionStore.getState().setConnState("ready")
      resolve()
    }

    gateway.chatTransport.onError = (err) => {
      clearTimeout(timeout)
      reject(err instanceof Error ? err : new Error(String(err)))
    }

    const selection = selectBrowserChatEndpoint(res.chat_endpoints, runtimeConfig.preferWebTransport)
    if (!runtimeConfig.useMockTransport && !selection) {
      throw new Error("Login response did not include a browser chat endpoint")
    }
    const primary = selection?.primary
    const websocketEndpoint = primary?.transport === "websocket" ? primary : selection?.websocketFallback
    const webtransportEndpoint = primary?.transport === "webtransport" ? primary : undefined

    const transport = primary?.transport === "webtransport" ? "webtransport" : "websocket"
    const serverInfo: ServerInfo = {
      transport,
      loginTicket: res.login_ticket,
      token: res.access_token,
      uid: res.uid,
      protocolVersion: res.protocol_version,
    }
    if (primary) {
      serverInfo.serverName = primary.server_name
    }
    if (websocketEndpoint) {
      serverInfo.websocketServerName = websocketEndpoint.server_name
    }
    if (webtransportEndpoint) {
      serverInfo.webtransportServerName = webtransportEndpoint.server_name
    }
    if (runtimeConfig.useMockTransport) {
      serverInfo.wsUrl = String(runtimeConfig.wsBridgeUrl)
    } else {
      if (websocketEndpoint) {
        serverInfo.wsUrl = buildEndpointUrl(websocketEndpoint)
      }
      if (webtransportEndpoint) {
        serverInfo.wtUrl = buildEndpointUrl(webtransportEndpoint)
      }
    }
    useSessionStore.getState().setConnState("chat_login")
    gateway.chatTransport.connect(serverInfo)
  })
}

async function sendRelationBootstrap(
  gateway: ReturnType<typeof getGateway>,
  uid: number,
): Promise<void> {
  return new Promise((resolve) => {
    let unsub = () => {}
    const timeout = setTimeout(() => {
      unsub()
      resolve()
    }, 3_000)
    unsub = gateway.dispatcher.subscribe(ReqId.ID_GET_RELATION_BOOTSTRAP_RSP, (frame) => {
      clearTimeout(timeout)
      unsub()
      const data = JSON.parse(frame.payload) as {
        friend_list?: Array<{
          uid?: number
          name?: string
          email?: string
          icon?: string
          user_id?: string
          nick?: string
          desc?: string
          back?: string
          labels?: string[]
        }>
        apply_list?: Array<{
          uid?: number
          from_uid?: number
          to_uid?: number
          apply_msg?: string
          status?: number
          apply_time?: number
          name?: string
          user_id?: string
          icon?: string
          nick?: string
          desc?: string
          labels?: string[]
        }>
      }
      if (data.friend_list) {
        useEntityStore.getState().setFriends(
          data.friend_list
            .map((f): Friend | null => {
              const friendUid = Number(f.uid ?? 0)
              if (friendUid <= 0) return null
              const displayName = (f.nick || f.name || normalizePublicUserId(f.user_id) || "未知用户").trim()
              const item: Friend = {
                uid: friendUid,
                name: displayName,
                email: (f.email || f.user_id || "").trim(),
                icon: f.icon || "",
              }
              if (f.user_id) item.userId = f.user_id
              if (f.nick) item.nick = f.nick
              if (f.desc) item.desc = f.desc
              if (f.back) item.back = f.back
              if (Array.isArray(f.labels)) item.labels = f.labels
              return item
            })
            .filter((f): f is Friend => f !== null),
        )
      }
      if (data.apply_list) {
        useEntityStore.getState().setApplyList(
          data.apply_list
            .map((a): ApplyEntry | null => {
              const fromUid = Number(a.from_uid ?? a.uid ?? 0)
              if (fromUid <= 0) return null
              const entry: ApplyEntry = {
                fromUid,
                applyMsg: a.apply_msg || a.desc || "请求添加你为好友",
                status: a.status === 1 ? "accepted" : a.status === 2 ? "rejected" : "pending",
                applyTime: Number(a.apply_time ?? Date.now()),
              }
              const toUid = Number(a.to_uid ?? 0)
              if (toUid > 0) entry.toUid = toUid
              if (a.name) entry.name = a.name
              if (a.user_id) entry.userId = a.user_id
              if (a.icon) entry.icon = a.icon
              if (a.nick) entry.nick = a.nick
              if (a.desc) entry.desc = a.desc
              if (Array.isArray(a.labels)) entry.labels = a.labels
              return entry
            })
            .filter((a): a is ApplyEntry => a !== null),
        )
      }
      resolve()
    })
    gateway.chatTransport.send(ReqId.ID_GET_RELATION_BOOTSTRAP_REQ, JSON.stringify({ uid }))
  })
}

async function sendGroupListBootstrap(
  gateway: ReturnType<typeof getGateway>,
  uid: number,
): Promise<void> {
  return new Promise((resolve) => {
    let unsub = () => {}
    const timeout = setTimeout(() => {
      unsub()
      resolve()
    }, 3_000)
    unsub = gateway.dispatcher.subscribe(ReqId.ID_GET_GROUP_LIST_RSP, (frame) => {
      clearTimeout(timeout)
      unsub()
      applyGroupListPayload(frame.payload)
      resolve()
    })
    gateway.chatTransport.send(ReqId.ID_GET_GROUP_LIST_REQ, JSON.stringify({ fromuid: uid }))
  })
}

async function sendDialogListBootstrap(
  gateway: ReturnType<typeof getGateway>,
  uid: number,
): Promise<void> {
  return new Promise((resolve) => {
    let unsub = () => {}
    const timeout = setTimeout(() => {
      unsub()
      resolve()
    }, 3_000)
    unsub = gateway.dispatcher.subscribe(ReqId.ID_GET_DIALOG_LIST_RSP, (frame) => {
      clearTimeout(timeout)
      unsub()
      applyDialogListPayload(frame.payload)
      resolve()
    })
    gateway.chatTransport.send(ReqId.ID_GET_DIALOG_LIST_REQ, JSON.stringify({ uid }))
  })
}
