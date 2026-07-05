/**
 * tokenStorage — security-conscious token persistence.
 *
 * Strategy (per architecture plan):
 * - token + loginTicket: memory-only (anti-XSS)
 * - refreshToken: sessionStorage only (no localStorage)
 * - loginTicket: never persisted (one-time use)
 */
const REFRESH_TOKEN_KEY = "mc_refresh_token";
export const tokenStorage = {
    saveRefreshToken(token) {
        try {
            sessionStorage.setItem(REFRESH_TOKEN_KEY, token);
        }
        catch {
            // private browsing may block storage writes
        }
    },
    loadRefreshToken() {
        try {
            return sessionStorage.getItem(REFRESH_TOKEN_KEY);
        }
        catch {
            return null;
        }
    },
    clearRefreshToken() {
        try {
            sessionStorage.removeItem(REFRESH_TOKEN_KEY);
        }
        catch {
            // no-op
        }
    },
    clearAll() {
        this.clearRefreshToken();
    },
};
