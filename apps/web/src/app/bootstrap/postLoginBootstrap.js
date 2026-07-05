/**
 * postLoginBootstrap — exact login sequence per architecture plan §5.
 * 1. HTTP login → store session
 * 2. WS connect → send chat-login (1005) → wait for 1006
 * 3. Send relation bootstrap (1092) → wait for 1093
 * 4. Start heartbeat
 * Errors: 1095/1096 → refresh; 1097 → re-fetch endpoints; 1098 → hard fail.
 */
import { useSessionStore } from "@/core/session/sessionStore";
import { useEntityStore } from "@/core/entities/entityStore";
import { ReqId } from "@/core/network/opcodes/reqIds";
import { ErrorCodes } from "@/core/network/opcodes/errorCodes";
import { runtimeConfig } from "@/core/config/runtimeConfig";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { logger } from "@/core/common/logger";
export function selectBrowserChatEndpoint(endpoints, preferWebTransport) {
    const byPriority = (left, right) => left.priority - right.priority;
    const websocket = endpoints
        .filter((ep) => ep.transport === "websocket")
        .sort(byPriority)[0];
    const webtransport = endpoints
        .filter((ep) => ep.transport === "webtransport")
        .sort(byPriority)[0];
    if (preferWebTransport && webtransport) {
        const selection = { primary: webtransport };
        if (websocket)
            selection.websocketFallback = websocket;
        return selection;
    }
    if (websocket) {
        return { primary: websocket };
    }
    return null;
}
/**
 * Resolve WebSocket protocol independently of the server-supplied tls flag.
 * A compromised or misconfigured server could set tls=false to silently
 * downgrade clients to plaintext. We ignore that hint for non-local hosts and
 * always use wss://, preventing credential theft over the wire (H-21).
 */
function resolveWsProtocol(host, serverTls) {
    // Always use wss in production (non-localhost)
    const isLocalhost = host === 'localhost' ||
        host === '127.0.0.1' ||
        host.startsWith('192.168.') ||
        host.endsWith('.local');
    if (!isLocalhost)
        return 'wss';
    // For localhost, respect server hint
    return serverTls ? 'wss' : 'ws';
}
export function buildEndpointUrl(endpoint) {
    const path = endpoint.path ?? (endpoint.transport === "webtransport" ? "/chat" : "/ws");
    if (endpoint.transport === "webtransport") {
        return `https://${endpoint.host}:${endpoint.port}${path}`;
    }
    const scheme = resolveWsProtocol(endpoint.host, endpoint.tls ?? false);
    return `${scheme}://${endpoint.host}:${endpoint.port}${path}`;
}
export async function postLoginBootstrap(creds) {
    const gateway = getGateway();
    const session = useSessionStore.getState();
    // Step 1: HTTP login
    const res = await gateway.http.post("/user_login", {
        email: creds.email,
        passwd: creds.password,
        client_ver: "3.0.0",
    }, { auth: false });
    const successCode = ErrorCodes.SUCCESS;
    if (res.error !== successCode) {
        throw new Error(`Login failed: error code ${res.error}`);
    }
    session.setLogin({
        uid: res.uid,
        token: res.token,
        loginTicket: res.login_ticket,
        ticketExpireMs: res.ticket_expire_ms,
        refreshToken: res.refresh_token,
        protocolVersion: res.protocol_version,
        chatEndpoints: res.chat_endpoints.map((ep) => {
            const endpoint = {
                transport: ep.transport,
                host: ep.host,
                port: ep.port,
                serverName: ep.server_name,
                priority: ep.priority,
            };
            if (ep.path !== undefined)
                endpoint.path = ep.path;
            if (ep.tls !== undefined)
                endpoint.tls = ep.tls;
            return endpoint;
        }),
        profile: {
            uid: res.user_profile.uid,
            name: res.user_profile.name,
            email: res.user_profile.email,
            icon: res.user_profile.icon,
        },
    });
    // Step 2: Connect WS transport
    session.setConnState("connecting");
    await connectAndChatLogin(gateway, res);
    // Step 3: Relation bootstrap
    await sendRelationBootstrap(gateway, res.uid);
    logger.app.info("Bootstrap complete for uid", res.uid);
}
async function connectAndChatLogin(gateway, res) {
    return new Promise((resolve, reject) => {
        const timeout = setTimeout(() => reject(new Error("Chat login timeout")), 10000);
        gateway.chatTransport.onReady = () => {
            clearTimeout(timeout);
            useSessionStore.getState().setConnState("ready");
            resolve();
        };
        gateway.chatTransport.onError = (err) => {
            clearTimeout(timeout);
            reject(err instanceof Error ? err : new Error(String(err)));
        };
        const selection = selectBrowserChatEndpoint(res.chat_endpoints, runtimeConfig.preferWebTransport);
        if (!runtimeConfig.useMockTransport && !selection) {
            throw new Error("Login response did not include a browser chat endpoint");
        }
        const primary = selection?.primary;
        const websocketEndpoint = primary?.transport === "websocket" ? primary : selection?.websocketFallback;
        const webtransportEndpoint = primary?.transport === "webtransport" ? primary : undefined;
        const transport = primary?.transport === "webtransport" ? "webtransport" : "websocket";
        const serverInfo = {
            transport,
            loginTicket: res.login_ticket,
            token: res.token,
            uid: res.uid,
            protocolVersion: res.protocol_version,
        };
        if (primary) {
            serverInfo.serverName = primary.server_name;
        }
        if (websocketEndpoint) {
            serverInfo.websocketServerName = websocketEndpoint.server_name;
        }
        if (webtransportEndpoint) {
            serverInfo.webtransportServerName = webtransportEndpoint.server_name;
        }
        if (runtimeConfig.useMockTransport) {
            serverInfo.wsUrl = String(runtimeConfig.wsBridgeUrl);
        }
        else {
            if (websocketEndpoint) {
                serverInfo.wsUrl = buildEndpointUrl(websocketEndpoint);
            }
            if (webtransportEndpoint) {
                serverInfo.wtUrl = buildEndpointUrl(webtransportEndpoint);
            }
        }
        useSessionStore.getState().setConnState("chat_login");
        gateway.chatTransport.connect(serverInfo);
    });
}
async function sendRelationBootstrap(gateway, uid) {
    return new Promise((resolve) => {
        const unsub = gateway.dispatcher.subscribe(ReqId.ID_GET_RELATION_BOOTSTRAP_RSP, (frame) => {
            unsub();
            const data = JSON.parse(frame.payload);
            if (data.friend_list) {
                useEntityStore.getState().setFriends(data.friend_list.map((f) => ({ uid: f.uid, name: f.name, email: f.email, icon: f.icon })));
            }
            if (data.apply_list) {
                useEntityStore.getState().setApplyList(data.apply_list.map((a) => ({
                    fromUid: a.from_uid,
                    toUid: a.to_uid,
                    applyMsg: a.apply_msg,
                    status: "pending",
                    applyTime: a.apply_time,
                })));
            }
            resolve();
        });
        setTimeout(resolve, 3000); // don't block on timeout
        gateway.chatTransport.send(ReqId.ID_GET_RELATION_BOOTSTRAP_REQ, JSON.stringify({ uid }));
    });
}
