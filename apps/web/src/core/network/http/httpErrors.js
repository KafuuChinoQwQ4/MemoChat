/** Typed HTTP errors aligned with the Envoy gateway */
export class RateLimitedError extends Error {
    constructor(message = "Rate limited (429)") {
        super(message);
        this.name = "RateLimitedError";
    }
}
export class HostNotAllowedError extends Error {
    constructor(message = "Host not allowed (421)") {
        super(message);
        this.name = "HostNotAllowedError";
    }
}
export class HttpError extends Error {
    constructor(status, message) {
        super(message);
        this.status = status;
        this.name = "HttpError";
    }
}
export function throwForStatus(response) {
    if (response.ok)
        return;
    if (response.status === 429)
        throw new RateLimitedError();
    if (response.status === 421)
        throw new HostNotAllowedError();
    throw new HttpError(response.status, `HTTP ${response.status} ${response.statusText}`);
}
