/**
 * sessionStore — Zustand store for authentication + connection state.
 * Mirrors ShellViewModel session fields + AppChatRecoveryState machine.
 * Memory-only for access token/loginTicket; refreshToken via tokenStorage.
 */
import { create } from "zustand";
import { tokenStorage } from "./tokenStorage";
export const useSessionStore = create((set, get) => ({
    // --- state ---
    uid: null,
    token: null,
    loginTicket: null,
    ticketExpireMs: null,
    refreshToken: null,
    protocolVersion: 3,
    chatEndpoints: [],
    profile: null,
    connState: "disconnected",
    heartbeatAckMissCount: 0,
    reconnectAttempts: 0,
    // --- actions ---
    setLogin(params) {
        tokenStorage.saveRefreshToken(params.refreshToken);
        set({
            uid: params.uid,
            token: params.token,
            loginTicket: params.loginTicket,
            ticketExpireMs: params.ticketExpireMs,
            refreshToken: params.refreshToken,
            protocolVersion: params.protocolVersion,
            chatEndpoints: params.chatEndpoints,
            profile: params.profile,
        });
    },
    setConnState: (state) => set({ connState: state }),
    setProfile: (profile) => set({ profile }),
    incrementMissCount: () => set((s) => ({ heartbeatAckMissCount: s.heartbeatAckMissCount + 1 })),
    resetMissCount: () => set({ heartbeatAckMissCount: 0 }),
    incrementReconnectAttempts: () => set((s) => ({ reconnectAttempts: s.reconnectAttempts + 1 })),
    clearSession() {
        tokenStorage.clearAll();
        set({
            uid: null,
            token: null,
            loginTicket: null,
            ticketExpireMs: null,
            refreshToken: null,
            chatEndpoints: [],
            profile: null,
            connState: "disconnected",
            heartbeatAckMissCount: 0,
            reconnectAttempts: 0,
        });
    },
    isLoggedIn: () => get().token !== null,
    isReady: () => get().connState === "ready",
}));
