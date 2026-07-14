/**
 * sessionStore — Zustand store for authentication + connection state.
 * Mirrors ShellViewModel session fields + AppChatRecoveryState machine.
 * Access token and loginTicket are memory-only. Page reload restore uses an
 * HttpOnly refresh cookie that is never exposed to this store.
 */
import { create } from "zustand"
import type { ConnState, SessionState, UserProfile, ChatEndpointInfo } from "./sessionTypes"

interface SessionActions {
  setLogin(params: {
    uid: number
    token: string
    loginTicket: string
    ticketExpireMs: number
    protocolVersion: number
    chatEndpoints: ChatEndpointInfo[]
    profile: UserProfile
  }): void
  setConnState(state: ConnState): void
  setProfile(profile: UserProfile): void
  incrementMissCount(): void
  resetMissCount(): void
  incrementReconnectAttempts(): void
  clearSession(): void
  // Selectors (computed, exposed for convenience)
  isLoggedIn(): boolean
  isReady(): boolean
}

export const useSessionStore = create<SessionState & SessionActions>((set, get) => ({
  // --- state ---
  uid: null,
  token: null,
  loginTicket: null,
  ticketExpireMs: null,
  protocolVersion: 3,
  chatEndpoints: [],
  profile: null,
  connState: "disconnected",
  heartbeatAckMissCount: 0,
  reconnectAttempts: 0,

  // --- actions ---
  setLogin(params) {
    set({
      uid: params.uid,
      token: params.token,
      loginTicket: params.loginTicket,
      ticketExpireMs: params.ticketExpireMs,
      protocolVersion: params.protocolVersion,
      chatEndpoints: params.chatEndpoints,
      profile: params.profile,
    })
  },

  setConnState: (state) => set({ connState: state }),

  setProfile: (profile) => set({ profile }),

  incrementMissCount: () =>
    set((s) => ({ heartbeatAckMissCount: s.heartbeatAckMissCount + 1 })),

  resetMissCount: () => set({ heartbeatAckMissCount: 0 }),

  incrementReconnectAttempts: () =>
    set((s) => ({ reconnectAttempts: s.reconnectAttempts + 1 })),

  clearSession() {
    set({
      uid: null,
      token: null,
      loginTicket: null,
      ticketExpireMs: null,
      chatEndpoints: [],
      profile: null,
      connState: "disconnected",
      heartbeatAckMissCount: 0,
      reconnectAttempts: 0,
    })
  },

  isLoggedIn: () => get().token !== null,
  isReady: () => get().connState === "ready",
}))
