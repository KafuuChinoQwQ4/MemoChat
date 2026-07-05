/** Minimal structured logger — wraps console, namespaced. */
const LOG_LEVEL = import.meta.env.DEV ? "debug" : "warn"
const LEVELS = { debug: 0, info: 1, warn: 2, error: 3 } as const
type Level = keyof typeof LEVELS

function shouldLog(level: Level): boolean {
  return LEVELS[level] >= LEVELS[LOG_LEVEL as Level ?? "warn"]
}

function makeLogger(ns: string) {
  return {
    debug: (...args: unknown[]) => shouldLog("debug") && console.debug(`[${ns}]`, ...args),
    info:  (...args: unknown[]) => shouldLog("info")  && console.info(`[${ns}]`, ...args),
    warn:  (...args: unknown[]) => shouldLog("warn")  && console.warn(`[${ns}]`, ...args),
    error: (...args: unknown[]) => shouldLog("error") && console.error(`[${ns}]`, ...args),
  }
}

export const logger = {
  transport: makeLogger("Transport"),
  dispatcher: makeLogger("Dispatcher"),
  session: makeLogger("Session"),
  chat: makeLogger("Chat"),
  auth: makeLogger("Auth"),
  http: makeLogger("HTTP"),
  sse: makeLogger("SSE"),
  entity: makeLogger("Entity"),
  app: makeLogger("App"),
}
