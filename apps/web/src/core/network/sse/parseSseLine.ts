/**
 * Parse a single Server-Sent Event line.
 * Returns { field, value } or null for blank/comment lines.
 */
export interface SseParsed {
  field: string
  value: string
}

export function parseSseLine(line: string): SseParsed | null {
  if (line.startsWith(":") || line.trim() === "") return null
  const colon = line.indexOf(":")
  if (colon === -1) return { field: line, value: "" }
  const field = line.slice(0, colon)
  const value = line.slice(colon + 1).replace(/^ /, "")
  return { field, value }
}
