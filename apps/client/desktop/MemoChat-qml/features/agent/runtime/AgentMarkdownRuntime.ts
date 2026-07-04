// Source — compiled to AgentMarkdownRuntime.js by `npm run build`.
// .pragma library is injected by esbuild as a banner.

// ─── Types ───────────────────────────────────────────────────────────────────

export interface Segment {
  kind: "text" | "code"
  text: string
  language: string
}

interface BlockComment {
  start: string
  end: string
}

interface SyntaxConfig {
  lineComments: string[]
  blockComments: BlockComment[]
  quotes: string[]
}

interface HighlightState {
  blockEnd: string
}

interface LatexCommand {
  name: string
  end: number
}

interface LatexGroup {
  text: string
  end: number
}

interface HighlightResult {
  html: string
  next: number
}

// ─── parseSegments ────────────────────────────────────────────────────────────

export function parseSegments(value: string): Segment[] {
  const input = value || ""
  const lines = input.replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n")
  const rows: Segment[] = []
  let buffer: string[] = []
  let code: string[] = []
  let inCode = false
  let language = ""

  function flushText(): void {
    if (buffer.length === 0) return
    const body = buffer.join("\n")
    if (body.length > 0) rows.push({ kind: "text", text: body, language: "" })
    buffer = []
  }

  function flushCode(): void {
    rows.push({ kind: "code", text: code.join("\n"), language })
    code = []
    language = ""
  }

  for (let i = 0; i < lines.length; ++i) {
    const line = lines[i]
    const trimmed = line.replace(/^\s+|\s+$/g, "")
    const isFence = trimmed.indexOf("```") === 0 || trimmed.indexOf("~~~") === 0
    if (isFence) {
      if (inCode) {
        flushCode()
        inCode = false
      } else {
        flushText()
        inCode = true
        language = trimmed.slice(3).replace(/^\s+|\s+$/g, "").split(/\s+/)[0] || ""
      }
    } else if (inCode) {
      code.push(line)
    } else {
      buffer.push(line)
    }
  }

  if (inCode) flushCode()
  flushText()
  return rows
}

// ─── Language normalisation ────────────────────────────────────────────────

export function normalizeLanguage(language: string): string {
  let raw = (language || "").toLowerCase().replace(/^\s+|\s+$/g, "")
  raw = raw.replace(/^language-/, "")
  const compact = raw.replace(/[^a-z0-9+#.\-]/g, "")
  if (compact === "js" || compact === "jsx" || compact === "node" || compact === "nodejs") return "javascript"
  if (compact === "ts" || compact === "tsx") return "typescript"
  if (compact === "py" || compact === "py3") return "python"
  if (compact === "c++" || compact === "cpp" || compact === "cc" || compact === "cxx" || compact === "hpp") return "cpp"
  if (compact === "c#" || compact === "cs") return "csharp"
  if (compact === "objc" || compact === "objectivec" || compact === "objective-c") return "objectivec"
  if (compact === "kt" || compact === "kts") return "kotlin"
  if (compact === "rs") return "rust"
  if (compact === "golang") return "go"
  if (compact === "sh" || compact === "bash" || compact === "zsh" || compact === "fish") return "shell"
  if (compact === "ps1" || compact === "pwsh" || compact === "powershell") return "powershell"
  if (compact === "yml") return "yaml"
  if (compact === "html" || compact === "htm" || compact === "xml" || compact === "xhtml" || compact === "svg") return "markup"
  if (compact === "scss" || compact === "sass" || compact === "less") return "css"
  if (compact === "md" || compact === "markdown") return "markdown"
  if (compact === "rb") return "ruby"
  if (compact === "pl" || compact === "perl") return "perl"
  if (compact === "r") return "r"
  if (compact === "lua") return "lua"
  if (compact === "dart") return "dart"
  if (compact === "scala") return "scala"
  if (compact === "sql" || compact === "mysql" || compact === "postgres" || compact === "postgresql") return "sql"
  if (compact === "dockerfile" || compact === "docker") return "dockerfile"
  if (compact === "cmake") return "cmake"
  if (compact === "json" || compact === "jsonc") return "json"
  if (compact === "qml") return "qml"
  return compact.length > 0 ? compact : "text"
}

export function languageLabel(language: string): string {
  const lang = normalizeLanguage(language)
  const labels: Record<string, string> = {
    cpp: "C++", csharp: "C#", javascript: "JavaScript", typescript: "TypeScript",
    python: "Python", shell: "Shell", powershell: "PowerShell", markup: "HTML/XML",
    objectivec: "Objective-C", dockerfile: "Dockerfile", json: "JSON", yaml: "YAML",
    sql: "SQL", qml: "QML",
  }
  return labels[lang] || (language && language.length > 0 ? language : "code")
}

// ─── HTML helpers ──────────────────────────────────────────────────────────

function escapeHtml(value: string): string {
  return (value || "")
    .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;")
}

function preserveSpace(value: string): string {
  return value.replace(/\t/g, "    ").replace(/ /g, "&nbsp;")
}

function span(color: string, value: string): string {
  return `<span style="color:${color};">${preserveSpace(escapeHtml(value))}</span>`
}

function plain(value: string): string {
  return preserveSpace(escapeHtml(value))
}

function startsWithAt(value: string, marker: string, index: number): boolean {
  return marker.length > 0 && value.substr(index, marker.length) === marker
}

// ─── Syntax highlighting ───────────────────────────────────────────────────

function syntaxConfig(lang: string): SyntaxConfig {
  let lineComments: string[] = []
  let blockComments: BlockComment[] = []
  const quotes: string[] = ['"', "'"]

  if (lang === "markup") {
    blockComments = [{ start: "<!--", end: "-->" }]
  } else if (["python","ruby","shell","powershell","yaml","r","perl"].includes(lang)) {
    lineComments = ["#"]
  } else if (lang === "sql" || lang === "lua") {
    lineComments = ["--"]
    blockComments = [{ start: "/*", end: "*/" }]
  } else if (lang !== "json" && lang !== "markdown") {
    lineComments = ["//"]
    blockComments = [{ start: "/*", end: "*/" }]
  }

  if (lang === "javascript" || lang === "typescript" || lang === "shell") {
    quotes.push("`")
  }
  return { lineComments, blockComments, quotes }
}

function keywordSource(lang: string): string {
  const commonLiterals = "true false null nil none undefined nan inf infinity yes no on off this self super"
  const commonTypes = "bool boolean byte char short int integer long float double decimal number string str object array list dict map set tuple void any unknown never symbol bigint date regexp"
  const jsWords = "abstract arguments async await break case catch class const continue debugger default delete do else enum export extends finally for from function get if implements import in instanceof interface let new of package private protected public return set static switch throw try typeof var void while with yield Promise Array Object Map Set WeakMap WeakSet Date Math JSON console window document"
  const cppWords = "auto break case char const continue default do double else enum extern float for goto if inline int long register restrict return short signed sizeof static struct switch typedef union unsigned void volatile while bool true false NULL alignas alignof and and_eq asm bitand bitor catch char8_t char16_t char32_t class compl concept constexpr consteval constinit const_cast co_await co_return co_yield decltype delete dynamic_cast explicit export false friend mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public reinterpret_cast requires static_assert static_cast template this thread_local throw true try typename typeid using virtual wchar_t xor xor_eq override final std string vector map unordered_map set unordered_set optional variant unique_ptr shared_ptr make_shared make_unique"
  const pyWords = "and as assert async await break class continue def del elif else except false finally for from global if import in is lambda nonlocal none not or pass raise return true try while with yield match case self cls list dict set tuple str int float bool object len range print open enumerate zip isinstance super dataclass"
  const sqlWords = "add all alter and any as asc backup between by case check column constraint create database default delete desc distinct drop exec exists foreign from full group having in index inner insert into is join key left like limit not null or order outer primary procedure right rownum select set table top truncate union unique update values view where with count avg sum min max"
  const shellWords = "if then else elif fi for while until do done case esac function in select time coproc break continue return exit export readonly local unset shift source alias test true false echo printf cd pwd read grep sed awk find xargs curl wget git docker kubectl npm pnpm yarn node python cmake"
  const luaWords = "and break do else elseif end false for function goto if in local nil not or repeat return then true until while ipairs pairs require print string table math coroutine"
  const cmakeWords = "add_compile_definitions add_compile_options add_custom_command add_custom_target add_definitions add_dependencies add_executable add_library add_subdirectory cmake_minimum_required configure_file enable_testing find_library find_package function if else elseif endif foreach endforeach include install link_directories list message option project return set target_compile_definitions target_compile_options target_include_directories target_link_libraries target_sources"
  const dockerWords = "FROM RUN CMD LABEL EXPOSE ENV ADD COPY ENTRYPOINT VOLUME USER WORKDIR ARG ONBUILD STOPSIGNAL HEALTHCHECK SHELL AS"

  let langWords = commonLiterals + " " + commonTypes
  if (lang === "javascript") langWords += " " + jsWords
  else if (lang === "typescript") langWords += " " + jsWords + " type keyof infer readonly namespace module declare implements interface public private protected abstract enum"
  else if (lang === "qml") langWords += " " + jsWords
  else if (lang === "cpp" || lang === "c") langWords += " " + cppWords
  else if (lang === "python") langWords += " " + pyWords
  else if (lang === "sql") langWords += " " + sqlWords
  else if (lang === "shell") langWords += " " + shellWords
  else if (lang === "lua") langWords += " " + luaWords
  else if (lang === "cmake") langWords += " " + cmakeWords
  else if (lang === "dockerfile") langWords += " " + dockerWords
  else langWords += " " + jsWords + " " + pyWords + " " + sqlWords
  return langWords
}

function keywordMap(lang: string, keywordCache: Record<string, Record<string, boolean>>): Record<string, boolean> {
  const key = normalizeLanguage(lang)
  if (keywordCache[key]) return keywordCache[key]
  const map: Record<string, boolean> = {}
  const words = keywordSource(key).split(/\s+/)
  for (const word of words) {
    if (word.length > 0) map[word.toLowerCase()] = true
  }
  keywordCache[key] = map
  return map
}

function isIdentifierStart(ch: string): boolean { return /[A-Za-z_$@]/.test(ch) }
function isIdentifierPart(ch: string): boolean { return /[A-Za-z0-9_$@\-]/.test(ch) }
function isDigit(ch: string): boolean { return /[0-9]/.test(ch) }

function highlightCodeRun(value: string, lang: string, keywordCache: Record<string, Record<string, boolean>>): string {
  const map = keywordMap(lang, keywordCache)
  let result = ""
  let i = 0
  while (i < value.length) {
    const ch = value.charAt(i)
    if (isIdentifierStart(ch)) {
      const start = i++
      while (i < value.length && isIdentifierPart(value.charAt(i))) i++
      const token = value.slice(start, i)
      result += map[token.toLowerCase()] ? span("#2f6fb3", token) : plain(token)
    } else if (isDigit(ch)) {
      const n = i++
      while (i < value.length && /[A-Za-z0-9_.]/.test(value.charAt(i))) i++
      result += span("#b75c3a", value.slice(n, i))
    } else if ("{}[]().,:;+-*/%=!<>|&^~?".indexOf(ch) >= 0) {
      result += span("#357a93", ch); i++
    } else {
      result += plain(ch); i++
    }
  }
  return result
}

function startsLineComment(line: string, index: number, markers: string[]): string {
  for (const m of markers) { if (startsWithAt(line, m, index)) return m }
  return ""
}

function startsBlockComment(line: string, index: number, markers: BlockComment[]): BlockComment | null {
  for (const m of markers) { if (startsWithAt(line, m.start, index)) return m }
  return null
}

function startsQuote(line: string, index: number, quotes: string[]): string {
  const ch = line.charAt(index)
  for (const q of quotes) { if (ch === q) return ch }
  return ""
}

function highlightString(line: string, index: number, quote: string): HighlightResult {
  let i = index + quote.length
  let escaped = false
  while (i < line.length) {
    const ch = line.charAt(i)
    if (!escaped && ch === "\\") { escaped = true; i++; continue }
    if (!escaped && ch === quote) { i++; break }
    escaped = false; i++
  }
  return { html: span("#4f7f45", line.slice(index, i)), next: i }
}

function highlightLine(line: string, lang: string, state: HighlightState, keywordCache: Record<string, Record<string, boolean>>): string {
  const config = syntaxConfig(lang)
  let result = ""
  let i = 0
  while (i < line.length) {
    if (state.blockEnd && state.blockEnd.length > 0) {
      const endIndex = line.indexOf(state.blockEnd, i)
      if (endIndex < 0) { result += span("#8593a7", line.slice(i)); return result }
      const commentEnd = endIndex + state.blockEnd.length
      result += span("#8593a7", line.slice(i, commentEnd))
      i = commentEnd; state.blockEnd = ""; continue
    }
    const lineMarker = startsLineComment(line, i, config.lineComments)
    if (lineMarker.length > 0) { result += span("#8593a7", line.slice(i)); break }
    const blockMarker = startsBlockComment(line, i, config.blockComments)
    if (blockMarker !== null) {
      const blockEnd = line.indexOf(blockMarker.end, i + blockMarker.start.length)
      if (blockEnd < 0) { state.blockEnd = blockMarker.end; result += span("#8593a7", line.slice(i)); break }
      const end = blockEnd + blockMarker.end.length
      result += span("#8593a7", line.slice(i, end)); i = end; continue
    }
    const quote = startsQuote(line, i, config.quotes)
    if (quote.length > 0) { const h = highlightString(line, i, quote); result += h.html; i = h.next; continue }
    let start = i
    while (i < line.length
      && startsLineComment(line, i, config.lineComments).length === 0
      && startsBlockComment(line, i, config.blockComments) === null
      && startsQuote(line, i, config.quotes).length === 0) { i++ }
    result += highlightCodeRun(line.slice(start, i), lang, keywordCache)
  }
  return result
}

export function highlightedCode(value: string, language: string, codeTextColor: string, keywordCache: Record<string, Record<string, boolean>>): string {
  const lang = normalizeLanguage(language)
  const state: HighlightState = { blockEnd: "" }
  const lines = (value || "").replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n")
  const html: string[] = []
  for (const line of lines) html.push(highlightLine(line, lang, state, keywordCache))
  const color = codeTextColor || "#314158"
  return `<html><body style="margin:0; padding:0; background:transparent; color:${color};">${html.join("<br/>")}</body></html>`
}

// ─── LaTeX ─────────────────────────────────────────────────────────────────

const LATEX_SYMBOLS: Record<string, string> = {
  alpha: "α", beta: "β", gamma: "γ", delta: "δ", epsilon: "ε", theta: "θ",
  lambda: "λ", mu: "μ", pi: "π", rho: "ρ", sigma: "σ", tau: "τ", phi: "φ",
  omega: "ω", Gamma: "Γ", Delta: "Δ", Theta: "Θ", Lambda: "Λ", Pi: "Π",
  Sigma: "Σ", Phi: "Φ", Omega: "Ω", times: "×", cdot: "·", pm: "±",
  le: "≤", leq: "≤", ge: "≥", geq: "≥", ne: "≠", neq: "≠", approx: "≈",
  infty: "∞", sum: "∑", prod: "∏", int: "∫", partial: "∂", nabla: "∇",
  rightarrow: "→", to: "→", leftarrow: "←", Rightarrow: "⇒", Leftarrow: "⇐",
}

function readLatexCommand(value: string, start: number): LatexCommand {
  let index = start + 1; let name = ""
  while (index < value.length && /[A-Za-z]/.test(value.charAt(index))) { name += value.charAt(index); index++ }
  if (name.length === 0 && index < value.length) { name = value.charAt(index); index++ }
  return { name, end: index }
}

function readLatexGroup(value: string, start: number): LatexGroup {
  if (start >= value.length) return { text: "", end: start }
  if (value.charAt(start) !== "{") {
    if (value.charAt(start) === "\\") {
      const command = readLatexCommand(value, start)
      return { text: value.slice(start, command.end), end: command.end }
    }
    return { text: value.charAt(start), end: start + 1 }
  }
  let depth = 0
  for (let i = start; i < value.length; ++i) {
    const ch = value.charAt(i)
    if (ch === "{") depth++
    else if (ch === "}") { depth--; if (depth === 0) return { text: value.slice(start + 1, i), end: i + 1 } }
  }
  return { text: value.slice(start + 1), end: value.length }
}

function skipLatexWhitespace(value: string, start: number): number {
  let i = start
  while (i < value.length && /\s/.test(value.charAt(i))) i++
  return i
}

function renderLatexTokens(value: string): string {
  const input = value || ""
  let html = ""; let i = 0
  while (i < input.length) {
    const ch = input.charAt(i)
    if (ch === "\\") {
      const command = readLatexCommand(input, i)
      if (command.name === "frac") {
        const ns = skipLatexWhitespace(input, command.end)
        const num = readLatexGroup(input, ns)
        const ds = skipLatexWhitespace(input, num.end)
        const den = readLatexGroup(input, ds)
        html += `<span style="white-space:nowrap;"><sup>${renderLatexTokens(num.text)}</sup>/<sub>${renderLatexTokens(den.text)}</sub></span>`
        i = den.end; continue
      }
      if (command.name === "sqrt") {
        const rs = skipLatexWhitespace(input, command.end)
        const root = readLatexGroup(input, rs)
        html += `√(<span style="text-decoration:overline;">${renderLatexTokens(root.text)}</span>)`
        i = root.end; continue
      }
      if (command.name === "left" || command.name === "right") { i = command.end; continue }
      html += Object.prototype.hasOwnProperty.call(LATEX_SYMBOLS, command.name)
        ? LATEX_SYMBOLS[command.name]
        : escapeHtml("\\" + command.name)
      i = command.end; continue
    }
    if (ch === "^" || ch === "_") {
      const gs = skipLatexWhitespace(input, i + 1)
      const group = readLatexGroup(input, gs)
      const tag = ch === "^" ? "sup" : "sub"
      html += `<${tag}>${renderLatexTokens(group.text)}</${tag}>`
      i = group.end; continue
    }
    if (ch === "{") { const nested = readLatexGroup(input, i); html += renderLatexTokens(nested.text); i = nested.end; continue }
    if (ch === "}") { i++; continue }
    html += escapeHtml(ch); i++
  }
  return html
}

function renderInlineMath(value: string): string {
  const formula = renderLatexTokens((value || "").replace(/\n/g, " "))
  return `<span style="font-family:'Times New Roman','Cambria Math',serif; color:#42526d; background:#eef4ff; border-radius:4px;">${formula}</span>`
}

function renderMathBlock(value: string): string {
  const formula = renderLatexTokens(value || "").replace(/\n/g, "<br/>")
  return `<div style="margin:7px 0; padding:8px 10px; text-align:center; background:#eef4ff; border:1px solid #d8e6ff; border-radius:7px;"><span style="font-family:'Times New Roman','Cambria Math',serif; color:#314158;">${formula}</span></div>`
}

// ─── Markdown rendering ────────────────────────────────────────────────────

function escapeAttribute(value: string): string {
  return escapeHtml(value).replace(/'/g, "&#39;")
}

function isAllowedHref(value: string): boolean {
  const text = (value || "").toLowerCase()
  return text.indexOf("http://") === 0 || text.indexOf("https://") === 0
    || text.indexOf("mailto:") === 0 || text.indexOf("qrc:/") === 0
}

function findUnescaped(value: string, marker: string, start: number): number {
  let index = start
  while (index < value.length) {
    index = value.indexOf(marker, index)
    if (index < 0) return -1
    let backslashes = 0; let cursor = index - 1
    while (cursor >= 0 && value.charAt(cursor) === "\\") { backslashes++; cursor-- }
    if (backslashes % 2 === 0) return index
    index += marker.length
  }
  return -1
}

function renderMarkdownRun(value: string): string {
  let html = escapeHtml(value || "")
  html = html.replace(/`([^`]+)`/g, `<span style="font-family:monospace; background:#eef2f8; color:#314158;">$1</span>`)
  html = html.replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, (_match, label, href) => {
    if (!isAllowedHref(href)) return label
    return `<a href="${escapeAttribute(href)}">${label}</a>`
  })
  html = html.replace(/\*\*([^*]+)\*\*/g, "<b>$1</b>")
  html = html.replace(/__([^_]+)__/g, "<b>$1</b>")
  html = html.replace(/~~([^~]+)~~/g, "<s>$1</s>")
  html = html.replace(/(^|[\s(])\*([^*\n]+)\*/g, "$1<i>$2</i>")
  html = html.replace(/(^|[\s(])_([^_\n]+)_/g, "$1<i>$2</i>")
  return html
}

function renderInlineMarkdown(value: string): string {
  const input = value || ""
  let result = ""; let normal = ""; let i = 0

  const flushNormal = () => {
    if (normal.length > 0) { result += renderMarkdownRun(normal); normal = "" }
  }

  while (i < input.length) {
    if (input.substr(i, 2) === "\\(") {
      const parenEnd = input.indexOf("\\)", i + 2)
      if (parenEnd >= 0) { flushNormal(); result += renderInlineMath(input.slice(i + 2, parenEnd)); i = parenEnd + 2; continue }
    }
    if (input.charAt(i) === "$" && input.charAt(i + 1) !== "$") {
      const dollarEnd = findUnescaped(input, "$", i + 1)
      if (dollarEnd >= 0) { flushNormal(); result += renderInlineMath(input.slice(i + 1, dollarEnd)); i = dollarEnd + 1; continue }
    }
    normal += input.charAt(i); i++
  }
  flushNormal()
  return result
}

function renderParagraph(lines: string[]): string {
  return `<p style="margin:0 0 6px 0;">${renderInlineMarkdown(lines.join(" ").replace(/\s+/g, " "))}</p>`
}

export function renderRichText(value: string): string {
  const input = value || ""
  const lines = input.replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n")
  const html: string[] = []
  let paragraph: string[] = []; let listItems: string[] = []
  let orderedList = false; let mathLines: string[] = []; let mathEndMarker = ""

  const flushParagraph = () => {
    if (paragraph.length > 0) { html.push(renderParagraph(paragraph)); paragraph = [] }
  }
  const flushList = () => {
    if (listItems.length === 0) return
    const tag = orderedList ? "ol" : "ul"
    html.push(`<${tag} style="margin:0 0 6px 18px; padding:0;">${listItems.join("")}</${tag}>`)
    listItems = []; orderedList = false
  }
  const flushMath = () => {
    if (mathLines.length > 0) { html.push(renderMathBlock(mathLines.join("\n"))); mathLines = [] }
    mathEndMarker = ""
  }

  for (const line of lines) {
    const trimmed = line.replace(/^\s+|\s+$/g, "")
    if (mathEndMarker.length > 0) {
      const mathEnd = trimmed.indexOf(mathEndMarker)
      if (mathEnd >= 0) {
        mathLines.push(trimmed.slice(0, mathEnd)); flushMath()
        const rest = trimmed.slice(mathEnd + mathEndMarker.length).replace(/^\s+/, "")
        if (rest.length > 0) paragraph.push(rest)
      } else { mathLines.push(line) }
      continue
    }
    if (trimmed.length === 0) { flushParagraph(); flushList(); continue }
    if (trimmed.indexOf("$$") === 0 || trimmed.indexOf("\\[") === 0) {
      flushParagraph(); flushList()
      const startMarker = trimmed.indexOf("$$") === 0 ? "$$" : "\\["
      mathEndMarker = startMarker === "$$" ? "$$" : "\\]"
      const body = trimmed.slice(startMarker.length)
      const inlineEnd = body.indexOf(mathEndMarker)
      inlineEnd >= 0 ? (mathLines.push(body.slice(0, inlineEnd)), flushMath()) : mathLines.push(body)
      continue
    }
    const heading = trimmed.match(/^(#{1,4})\s+(.+)$/)
    if (heading) {
      flushParagraph(); flushList()
      const size = Math.max(14, 20 - heading[1].length)
      html.push(`<h3 style="margin:4px 0 6px 0; font-size:${size}px;">${renderInlineMarkdown(heading[2])}</h3>`)
      continue
    }
    const quote = trimmed.match(/^>\s?(.*)$/)
    if (quote) {
      flushParagraph(); flushList()
      html.push(`<blockquote style="margin:0 0 6px 0; padding-left:9px; color:#5d6b7f; border-left:3px solid #c5d4ea;">${renderInlineMarkdown(quote[1])}</blockquote>`)
      continue
    }
    const bullet = trimmed.match(/^[-*+]\s+(.+)$/)
    const ordered = trimmed.match(/^\d+[.)]\s+(.+)$/)
    if (bullet || ordered) {
      flushParagraph()
      const nextOrdered = !!ordered
      if (listItems.length > 0 && orderedList !== nextOrdered) flushList()
      orderedList = nextOrdered
      listItems.push(`<li>${renderInlineMarkdown((bullet || ordered)![1])}</li>`)
      continue
    }
    paragraph.push(line)
  }

  flushParagraph(); flushList(); flushMath()
  return `<html><body style="margin:0; padding:0; background:transparent;">${html.join("")}</body></html>`
}
