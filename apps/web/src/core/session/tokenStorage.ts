/**
 * tokenStorage — security-conscious token persistence.
 *
 * Strategy (per architecture plan):
 * - access token + loginTicket: memory-only (anti-XSS)
 * - refreshToken: sessionStorage only (no localStorage)
 * - loginTicket: never persisted (one-time use)
 */

const REFRESH_TOKEN_KEY = "mc_refresh_token"

export const tokenStorage = {
  saveRefreshToken(token: string): void {
    try {
      sessionStorage.setItem(REFRESH_TOKEN_KEY, token)
    } catch {
      // private browsing may block storage writes
    }
  },

  loadRefreshToken(): string | null {
    try {
      return sessionStorage.getItem(REFRESH_TOKEN_KEY)
    } catch {
      return null
    }
  },

  clearRefreshToken(): void {
    try {
      sessionStorage.removeItem(REFRESH_TOKEN_KEY)
    } catch {
      // no-op
    }
  },

  clearAll(): void {
    this.clearRefreshToken()
  },
}
