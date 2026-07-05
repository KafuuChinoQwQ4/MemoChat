/**
 * connectionCoordinator — subscribes to WS lifecycle events and manages
 * reconnect state in sessionStore. Called once after post-login bootstrap.
 */
import { useSessionStore } from "@/core/session/sessionStore";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { postLoginBootstrap } from "./postLoginBootstrap";
import { logger } from "@/core/common/logger";
export function startConnectionCoordinator() {
    const gateway = getGateway();
    const prevOnClose = gateway.chatTransport.onClose;
    const prevOnError = gateway.chatTransport.onError;
    gateway.chatTransport.onClose = (reason) => {
        prevOnClose?.(reason);
        const s = useSessionStore.getState();
        if (s.uid !== null) {
            s.setConnState("reconnecting");
            logger.transport.warn("WS closed, will reconnect:", reason);
        }
    };
    gateway.chatTransport.onError = (err) => {
        prevOnError?.(err);
        logger.transport.error("WS error:", err);
    };
    // Restore previous handlers on teardown
    return () => {
        if (prevOnClose)
            gateway.chatTransport.onClose = prevOnClose;
        if (prevOnError)
            gateway.chatTransport.onError = prevOnError;
    };
}
