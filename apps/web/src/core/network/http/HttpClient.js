/**
 * HttpClient — mirrors HttpMgr, but as an injectable instance (not singleton).
 * All feature api/ modules receive one through ClientGateway.
 */
import { throwForStatus } from "./httpErrors";
import { buildTraceHeaders } from "./traceHeaders";
import { runtimeConfig } from "@/core/config/runtimeConfig";
export class HttpClient {
    constructor(_baseUrl = runtimeConfig.gateBaseUrl, 
    /** Returns the current session token, or null if not logged in */
    _getToken = () => null) {
        this._baseUrl = _baseUrl;
        this._getToken = _getToken;
    }
    async post(path, body, opts = {}) {
        const headers = {
            "Content-Type": "application/json",
            ...buildTraceHeaders(),
            ...opts.headers,
        };
        if (opts.auth !== false) {
            const token = this._getToken();
            if (token)
                headers["Authorization"] = `Bearer ${token}`;
        }
        const res = await fetch(`${this._baseUrl}${path}`, {
            method: "POST",
            headers,
            body: JSON.stringify(body),
            signal: opts.signal ?? null,
        });
        throwForStatus(res);
        return res.json();
    }
    async get(path, opts = {}) {
        const headers = {
            ...buildTraceHeaders(),
            ...opts.headers,
        };
        if (opts.auth !== false) {
            const token = this._getToken();
            if (token)
                headers["Authorization"] = `Bearer ${token}`;
        }
        const res = await fetch(`${this._baseUrl}${path}`, {
            method: "GET",
            headers,
            signal: opts.signal ?? null,
        });
        throwForStatus(res);
        return res.json();
    }
}
