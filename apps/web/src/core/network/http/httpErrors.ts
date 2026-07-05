/** Typed HTTP errors aligned with the Envoy gateway */

export class RateLimitedError extends Error {
  override readonly name = "RateLimitedError"
  constructor(message = "Rate limited (429)") { super(message) }
}

export class HostNotAllowedError extends Error {
  override readonly name = "HostNotAllowedError"
  constructor(message = "Host not allowed (421)") { super(message) }
}

export class HttpError extends Error {
  override readonly name = "HttpError"
  constructor(
    public readonly status: number,
    message: string,
  ) { super(message) }
}

export function throwForStatus(response: Response): void {
  if (response.ok) return
  if (response.status === 429) throw new RateLimitedError()
  if (response.status === 421) throw new HostNotAllowedError()
  throw new HttpError(response.status, `HTTP ${response.status} ${response.statusText}`)
}
