/**
 * HttpClient — mirrors HttpMgr, but as an injectable instance (not singleton).
 * All feature api/ modules receive one through ClientGateway.
 */
import { throwForStatus } from "./httpErrors"
import { buildTraceHeaders } from "./traceHeaders"
import { runtimeConfig } from "@/core/config/runtimeConfig"

export interface RequestOptions {
  headers?: Record<string, string>
  signal?: AbortSignal
  /** If true, attach Authorization: Bearer <token> from sessionStore */
  auth?: boolean
}

export class HttpClient {
  constructor(
    private readonly _baseUrl = runtimeConfig.gateBaseUrl,
    /** Returns the current session token, or null if not logged in */
    private readonly _getToken: () => string | null = () => null,
  ) {}

  async post<T = unknown>(path: string, body: unknown, opts: RequestOptions = {}): Promise<T> {
    const headers: Record<string, string> = {
      "Content-Type": "application/json",
      ...buildTraceHeaders(),
      ...opts.headers,
    }
    if (opts.auth !== false) {
      const token = this._getToken()
      if (token) headers["Authorization"] = `Bearer ${token}`
    }
    const res = await fetch(`${this._baseUrl}${path}`, {
      method: "POST",
      headers,
      body: JSON.stringify(body),
      signal: opts.signal ?? null,
    })
    throwForStatus(res)
    return res.json() as Promise<T>
  }

  async get<T = unknown>(path: string, opts: RequestOptions = {}): Promise<T> {
    const headers: Record<string, string> = {
      ...buildTraceHeaders(),
      ...opts.headers,
    }
    if (opts.auth !== false) {
      const token = this._getToken()
      if (token) headers["Authorization"] = `Bearer ${token}`
    }
    const res = await fetch(`${this._baseUrl}${path}`, {
      method: "GET",
      headers,
      signal: opts.signal ?? null,
    })
    throwForStatus(res)
    return res.json() as Promise<T>
  }
}
