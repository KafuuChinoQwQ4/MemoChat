.pragma library

function parseSegments(value) {
    var input = value || ""
    var lines = input.replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n")
    var rows = []
    var buffer = []
    var code = []
    var inCode = false
    var language = ""

    function flushText() {
        if (buffer.length === 0) {
            return
        }
        var body = buffer.join("\n")
        if (body.length > 0) {
            rows.push({ "kind": "text", "text": body, "language": "" })
        }
        buffer = []
    }

    function flushCode() {
        rows.push({ "kind": "code", "text": code.join("\n"), "language": language })
        code = []
        language = ""
    }

    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i]
        var trimmed = line.replace(/^\s+|\s+$/g, "")
        var isFence = trimmed.indexOf("```") === 0 || trimmed.indexOf("~~~") === 0
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

    if (inCode) {
        flushCode()
    }
    flushText()
    return rows
}

function normalizeLanguage(language) {
    var raw = (language || "").toLowerCase().replace(/^\s+|\s+$/g, "")
    raw = raw.replace(/^language-/, "")
    var compact = raw.replace(/[^a-z0-9+#.\-]/g, "")
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

function languageLabel(language) {
    var lang = normalizeLanguage(language)
    var labels = {
        "cpp": "C++",
        "csharp": "C#",
        "javascript": "JavaScript",
        "typescript": "TypeScript",
        "python": "Python",
        "shell": "Shell",
        "powershell": "PowerShell",
        "markup": "HTML/XML",
        "objectivec": "Objective-C",
        "dockerfile": "Dockerfile",
        "json": "JSON",
        "yaml": "YAML",
        "sql": "SQL",
        "qml": "QML"
    }
    return labels[lang] || (language && language.length > 0 ? language : "code")
}

function escapeHtml(value) {
    return (value || "")
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
}

function preserveSpace(value) {
    return value.replace(/\t/g, "    ").replace(/ /g, "&nbsp;")
}

function span(color, value) {
    return "<span style=\"color:" + color + ";\">" + preserveSpace(escapeHtml(value)) + "</span>"
}

function plain(value) {
    return preserveSpace(escapeHtml(value))
}

function startsWithAt(value, marker, index) {
    return marker.length > 0 && value.substr(index, marker.length) === marker
}

function syntaxConfig(lang) {
    var lineComments = []
    var blockComments = []
    var quotes = ["\"", "'"]

    if (lang === "markup") {
        blockComments = [{ "start": "<!--", "end": "-->" }]
    } else if (lang === "python" || lang === "ruby" || lang === "shell" || lang === "powershell" || lang === "yaml" || lang === "r" || lang === "perl") {
        lineComments = ["#"]
    } else if (lang === "sql" || lang === "lua") {
        lineComments = ["--"]
        blockComments = [{ "start": "/*", "end": "*/" }]
    } else if (lang === "json") {
        lineComments = []
    } else if (lang === "markdown") {
        lineComments = []
    } else {
        lineComments = ["//"]
        blockComments = [{ "start": "/*", "end": "*/" }]
    }

    if (lang === "javascript" || lang === "typescript" || lang === "shell") {
        quotes.push("`")
    }
    return { "lineComments": lineComments, "blockComments": blockComments, "quotes": quotes }
}

function keywordSource(lang) {
    var commonLiterals = "true false null nil none undefined nan inf infinity yes no on off this self super"
    var commonTypes = "bool boolean byte char short int integer long float double decimal number string str object array list dict map set tuple void any unknown never symbol bigint date regexp"
    var qmlWords = "import property signal function readonly required alias var let const on anchors parent children width height x y z opacity visible enabled focus color radius border font model delegate sourceComponent source text role states transitions Behavior NumberAnimation ColorAnimation Rectangle Item Text Label TextEdit TextArea TextField Image MouseArea Flickable ListView GridView Repeater Loader Component Column Row ColumnLayout RowLayout GridLayout ScrollView ScrollBar ComboBox Button CheckBox Menu MenuItem Timer Connections Qt"
    var jsWords = "abstract arguments async await break case catch class const continue debugger default delete do else enum export extends finally for from function get if implements import in instanceof interface let new of package private protected public return set static switch throw try typeof var void while with yield Promise Array Object Map Set WeakMap WeakSet Date Math JSON console window document"
    var cWords = "auto break case char const continue default do double else enum extern float for goto if inline int long register restrict return short signed sizeof static struct switch typedef union unsigned void volatile while bool true false NULL"
    var cppWords = cWords + " alignas alignof and and_eq asm bitand bitor catch char8_t char16_t char32_t class compl concept constexpr consteval constinit const_cast co_await co_return co_yield decltype delete dynamic_cast explicit export false friend mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public reinterpret_cast requires static_assert static_cast template this thread_local throw true try typename typeid using virtual wchar_t xor xor_eq override final std string vector map unordered_map set unordered_set optional variant unique_ptr shared_ptr make_shared make_unique"
    var csharpWords = "abstract as base bool break byte case catch char checked class const continue decimal default delegate do double dynamic else enum event explicit extern false finally fixed float for foreach goto if implicit in int interface internal is lock long namespace new null object operator out override params private protected public readonly ref return sbyte sealed short sizeof stackalloc static string struct switch this throw true try typeof uint ulong unchecked unsafe ushort using virtual void volatile while async await var record init get set value yield"
    var javaWords = "abstract assert boolean break byte case catch char class const continue default do double else enum extends final finally float for goto if implements import instanceof int interface long native new null package private protected public return short static strictfp super switch synchronized this throw throws transient true try void volatile while var record sealed permits"
    var pyWords = "and as assert async await break class continue def del elif else except false finally for from global if import in is lambda nonlocal none not or pass raise return true try while with yield match case self cls list dict set tuple str int float bool object len range print open enumerate zip isinstance super dataclass"
    var goWords = "break default func interface select case defer go map struct chan else goto package switch const fallthrough if range type continue for import return var nil true false string int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 uintptr byte rune float32 float64 complex64 complex128 bool error make new append cap close copy delete len panic recover"
    var rustWords = "as async await break const continue crate dyn else enum extern false fn for if impl in let loop match mod move mut pub ref return self Self static struct super trait true type unsafe use where while abstract become box do final macro override priv try typeof unsized virtual yield Option Result Some None Ok Err String Vec Box HashMap"
    var sqlWords = "add all alter and any as asc backup between by case check column constraint create database default delete desc distinct drop exec exists foreign from full group having in index inner insert into is join key left like limit not null or order outer primary procedure right rownum select set table top truncate union unique update values view where with count avg sum min max"
    var cssWords = "align animation background border box color content display flex font grid height justify left margin max min none padding position relative absolute fixed right top transform transition width z-index important media keyframes hover active focus root var calc rgba linear-gradient block inline inline-block hidden visible auto center"
    var shellWords = "if then else elif fi for while until do done case esac function in select time coproc break continue return exit export readonly local unset shift source alias test true false echo printf cd pwd read grep sed awk find xargs curl wget git docker kubectl npm pnpm yarn node python cmake"
    var psWords = "begin break catch class continue data define do dynamicparam else elseif end enum exit filter finally for foreach from function if in param process return switch throw trap try until using var while workflow true false null Write-Host Write-Output Get-ChildItem Set-Location Select-String Where-Object ForEach-Object Start-Process New-Item Remove-Item Copy-Item Move-Item"
    var markupWords = "html head body div span section article main header footer nav aside script style link meta title input button form label table thead tbody tr td th ul ol li p a img svg path rect circle g defs template slot canvas video audio"
    var kotlinWords = "as break class continue do else false for fun if in interface is null object package return super this throw true try typealias typeof val var when while by catch constructor delegate dynamic field file finally get import init param property receiver set setparam where actual abstract annotation companion const crossinline data enum expect external final infix inline inner internal lateinit noinline open operator out override private protected public reified sealed suspend tailrec vararg"
    var swiftWords = "associatedtype class deinit enum extension fileprivate func import init inout internal let open operator private protocol public static struct subscript typealias var break case continue default defer do else fallthrough for guard if in repeat return switch where while as any catch false is nil rethrows super self Self throw throws true try async await actor"
    var phpWords = "abstract and array as break callable case catch class clone const continue declare default die do echo else elseif empty enddeclare endfor endforeach endif endswitch endwhile enum eval exit extends final finally fn for foreach function global goto if implements include include_once instanceof insteadof interface isset list match namespace new or print private protected public readonly require require_once return static switch throw trait try unset use var while xor yield true false null"
    var rubyWords = "begin break case class def defined do else elsif end ensure false for if in module next nil not or redo rescue retry return self super then true undef unless until when while yield alias and BEGIN END require include extend attr_reader attr_writer attr_accessor puts print"
    var luaWords = "and break do else elseif end false for function goto if in local nil not or repeat return then true until while ipairs pairs require print string table math coroutine"
    var dartWords = "abstract as assert async await break case catch class const continue covariant default deferred do dynamic else enum export extends extension external factory false final finally for Function get hide if implements import in interface is late library mixin new null on operator part required rethrow return set show static super switch sync this throw true try typedef var void while with yield"
    var scalaWords = "abstract case catch class def do else enum export extends false final finally for forSome given if implicit import lazy match new null object override package private protected return sealed super then this throw trait true try type val var while with yield using"
    var cmakeWords = "add_compile_definitions add_compile_options add_custom_command add_custom_target add_definitions add_dependencies add_executable add_library add_subdirectory cmake_minimum_required configure_file enable_testing find_library find_package function if else elseif endif foreach endforeach include install link_directories list message option project return set target_compile_definitions target_compile_options target_include_directories target_link_libraries target_sources"
    var dockerWords = "FROM RUN CMD LABEL EXPOSE ENV ADD COPY ENTRYPOINT VOLUME USER WORKDIR ARG ONBUILD STOPSIGNAL HEALTHCHECK SHELL AS"
    var langWords = commonLiterals + " " + commonTypes

    if (lang === "javascript") langWords += " " + jsWords
    else if (lang === "typescript") langWords += " " + jsWords + " type keyof infer readonly namespace module declare implements interface public private protected abstract enum"
    else if (lang === "qml") langWords += " " + jsWords + " " + qmlWords
    else if (lang === "c") langWords += " " + cWords
    else if (lang === "cpp") langWords += " " + cppWords
    else if (lang === "csharp") langWords += " " + csharpWords
    else if (lang === "java") langWords += " " + javaWords
    else if (lang === "python") langWords += " " + pyWords
    else if (lang === "go") langWords += " " + goWords
    else if (lang === "rust") langWords += " " + rustWords
    else if (lang === "sql") langWords += " " + sqlWords
    else if (lang === "css") langWords += " " + cssWords
    else if (lang === "shell") langWords += " " + shellWords
    else if (lang === "powershell") langWords += " " + psWords
    else if (lang === "markup") langWords += " " + markupWords
    else if (lang === "kotlin") langWords += " " + kotlinWords
    else if (lang === "swift") langWords += " " + swiftWords
    else if (lang === "php") langWords += " " + phpWords
    else if (lang === "ruby") langWords += " " + rubyWords
    else if (lang === "lua") langWords += " " + luaWords
    else if (lang === "dart") langWords += " " + dartWords
    else if (lang === "scala") langWords += " " + scalaWords
    else if (lang === "cmake") langWords += " " + cmakeWords
    else if (lang === "dockerfile") langWords += " " + dockerWords
    else if (lang === "json" || lang === "yaml") langWords += " true false null"
    else langWords += " " + jsWords + " " + pyWords + " " + sqlWords

    return langWords
}

function keywordMap(lang, keywordCache) {
    var cache = keywordCache || {}
    var key = normalizeLanguage(lang)
    if (cache[key]) {
        return cache[key]
    }
    var map = {}
    var words = keywordSource(key).split(/\s+/)
    for (var i = 0; i < words.length; ++i) {
        if (words[i].length > 0) {
            map[words[i].toLowerCase()] = true
        }
    }
    cache[key] = map
    return map
}

function isIdentifierStart(ch) {
    return /[A-Za-z_$@]/.test(ch)
}

function isIdentifierPart(ch) {
    return /[A-Za-z0-9_$@\-]/.test(ch)
}

function isDigit(ch) {
    return /[0-9]/.test(ch)
}

function highlightCodeRun(value, lang, keywordCache) {
    var map = keywordMap(lang, keywordCache)
    var result = ""
    var i = 0
    while (i < value.length) {
        var ch = value.charAt(i)
        if (isIdentifierStart(ch)) {
            var start = i
            ++i
            while (i < value.length && isIdentifierPart(value.charAt(i))) {
                ++i
            }
            var token = value.slice(start, i)
            if (map[token.toLowerCase()]) {
                result += span("#2f6fb3", token)
            } else {
                result += plain(token)
            }
        } else if (isDigit(ch)) {
            var n = i
            ++i
            while (i < value.length && /[A-Za-z0-9_.]/.test(value.charAt(i))) {
                ++i
            }
            result += span("#b75c3a", value.slice(n, i))
        } else if ("{}[]().,:;+-*/%=!<>|&^~?".indexOf(ch) >= 0) {
            result += span("#357a93", ch)
            ++i
        } else {
            result += plain(ch)
            ++i
        }
    }
    return result
}

function startsLineComment(line, index, markers) {
    for (var i = 0; i < markers.length; ++i) {
        if (startsWithAt(line, markers[i], index)) {
            return markers[i]
        }
    }
    return ""
}

function startsBlockComment(line, index, markers) {
    for (var i = 0; i < markers.length; ++i) {
        if (startsWithAt(line, markers[i].start, index)) {
            return markers[i]
        }
    }
    return null
}

function startsQuote(line, index, quotes) {
    var ch = line.charAt(index)
    for (var i = 0; i < quotes.length; ++i) {
        if (ch === quotes[i]) {
            return ch
        }
    }
    return ""
}

function highlightString(line, index, quote) {
    var i = index + quote.length
    var escaped = false
    while (i < line.length) {
        var ch = line.charAt(i)
        if (!escaped && ch === "\\") {
            escaped = true
            ++i
            continue
        }
        if (!escaped && ch === quote) {
            ++i
            break
        }
        escaped = false
        ++i
    }
    return { "html": span("#4f7f45", line.slice(index, i)), "next": i }
}

function highlightLine(line, lang, state, keywordCache) {
    var config = syntaxConfig(lang)
    var result = ""
    var i = 0

    while (i < line.length) {
        if (state.blockEnd && state.blockEnd.length > 0) {
            var endIndex = line.indexOf(state.blockEnd, i)
            if (endIndex < 0) {
                result += span("#8593a7", line.slice(i))
                return result
            }
            var commentEnd = endIndex + state.blockEnd.length
            result += span("#8593a7", line.slice(i, commentEnd))
            i = commentEnd
            state.blockEnd = ""
            continue
        }

        var lineMarker = startsLineComment(line, i, config.lineComments)
        if (lineMarker.length > 0) {
            result += span("#8593a7", line.slice(i))
            break
        }

        var blockMarker = startsBlockComment(line, i, config.blockComments)
        if (blockMarker !== null) {
            var blockEnd = line.indexOf(blockMarker.end, i + blockMarker.start.length)
            if (blockEnd < 0) {
                state.blockEnd = blockMarker.end
                result += span("#8593a7", line.slice(i))
                break
            }
            var end = blockEnd + blockMarker.end.length
            result += span("#8593a7", line.slice(i, end))
            i = end
            continue
        }

        var quote = startsQuote(line, i, config.quotes)
        if (quote.length > 0) {
            var highlighted = highlightString(line, i, quote)
            result += highlighted.html
            i = highlighted.next
            continue
        }

        var start = i
        while (i < line.length
               && startsLineComment(line, i, config.lineComments).length === 0
               && startsBlockComment(line, i, config.blockComments) === null
               && startsQuote(line, i, config.quotes).length === 0) {
            ++i
        }
        result += highlightCodeRun(line.slice(start, i), lang, keywordCache)
    }

    return result
}

function highlightedCode(value, language, codeTextColor, keywordCache) {
    var lang = normalizeLanguage(language)
    var state = { "blockEnd": "" }
    var lines = (value || "").replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n")
    var html = []
    for (var i = 0; i < lines.length; ++i) {
        html.push(highlightLine(lines[i], lang, state, keywordCache))
    }
    var color = codeTextColor || "#314158"
    return "<html><body style=\"margin:0; padding:0; background:transparent; color:" + color + ";\">" + html.join("<br/>") + "</body></html>"
}

function escapeAttribute(value) {
    return escapeHtml(value).replace(/'/g, "&#39;")
}

function isAllowedHref(value) {
    var text = (value || "").toLowerCase()
    return text.indexOf("http://") === 0
        || text.indexOf("https://") === 0
        || text.indexOf("mailto:") === 0
        || text.indexOf("qrc:/") === 0
}

function findUnescaped(value, marker, start) {
    var index = start
    while (index < value.length) {
        index = value.indexOf(marker, index)
        if (index < 0) {
            return -1
        }
        var backslashes = 0
        var cursor = index - 1
        while (cursor >= 0 && value.charAt(cursor) === "\\") {
            ++backslashes
            --cursor
        }
        if (backslashes % 2 === 0) {
            return index
        }
        index += marker.length
    }
    return -1
}

var LATEX_SYMBOLS = {
    "alpha": "α",
    "beta": "β",
    "gamma": "γ",
    "delta": "δ",
    "epsilon": "ε",
    "theta": "θ",
    "lambda": "λ",
    "mu": "μ",
    "pi": "π",
    "rho": "ρ",
    "sigma": "σ",
    "tau": "τ",
    "phi": "φ",
    "omega": "ω",
    "Gamma": "Γ",
    "Delta": "Δ",
    "Theta": "Θ",
    "Lambda": "Λ",
    "Pi": "Π",
    "Sigma": "Σ",
    "Phi": "Φ",
    "Omega": "Ω",
    "times": "×",
    "cdot": "·",
    "pm": "±",
    "le": "≤",
    "leq": "≤",
    "ge": "≥",
    "geq": "≥",
    "ne": "≠",
    "neq": "≠",
    "approx": "≈",
    "infty": "∞",
    "sum": "∑",
    "prod": "∏",
    "int": "∫",
    "partial": "∂",
    "nabla": "∇",
    "rightarrow": "→",
    "to": "→",
    "leftarrow": "←",
    "Rightarrow": "⇒",
    "Leftarrow": "⇐"
}

function readLatexCommand(value, start) {
    var index = start + 1
    var name = ""
    while (index < value.length && /[A-Za-z]/.test(value.charAt(index))) {
        name += value.charAt(index)
        ++index
    }
    if (name.length === 0 && index < value.length) {
        name = value.charAt(index)
        ++index
    }
    return { "name": name, "end": index }
}

function readLatexGroup(value, start) {
    if (start >= value.length) {
        return { "text": "", "end": start }
    }
    if (value.charAt(start) !== "{") {
        if (value.charAt(start) === "\\") {
            var command = readLatexCommand(value, start)
            return { "text": value.slice(start, command.end), "end": command.end }
        }
        return { "text": value.charAt(start), "end": start + 1 }
    }

    var depth = 0
    for (var i = start; i < value.length; ++i) {
        var ch = value.charAt(i)
        if (ch === "{") {
            ++depth
        } else if (ch === "}") {
            --depth
            if (depth === 0) {
                return { "text": value.slice(start + 1, i), "end": i + 1 }
            }
        }
    }
    return { "text": value.slice(start + 1), "end": value.length }
}

function skipLatexWhitespace(value, start) {
    var index = start
    while (index < value.length && /\s/.test(value.charAt(index))) {
        ++index
    }
    return index
}

function renderLatexTokens(value) {
    var input = value || ""
    var html = ""
    var i = 0

    while (i < input.length) {
        var ch = input.charAt(i)
        if (ch === "\\") {
            var command = readLatexCommand(input, i)
            if (command.name === "frac") {
                var numeratorStart = skipLatexWhitespace(input, command.end)
                var numerator = readLatexGroup(input, numeratorStart)
                var denominatorStart = skipLatexWhitespace(input, numerator.end)
                var denominator = readLatexGroup(input, denominatorStart)
                html += "<span style=\"white-space:nowrap;\"><sup>" + renderLatexTokens(numerator.text)
                    + "</sup>/<sub>" + renderLatexTokens(denominator.text) + "</sub></span>"
                i = denominator.end
                continue
            }
            if (command.name === "sqrt") {
                var rootStart = skipLatexWhitespace(input, command.end)
                var root = readLatexGroup(input, rootStart)
                html += "√(<span style=\"text-decoration:overline;\">" + renderLatexTokens(root.text) + "</span>)"
                i = root.end
                continue
            }
            if (command.name === "left" || command.name === "right") {
                i = command.end
                continue
            }
            if (Object.prototype.hasOwnProperty.call(LATEX_SYMBOLS, command.name)) {
                html += LATEX_SYMBOLS[command.name]
            } else {
                html += escapeHtml("\\" + command.name)
            }
            i = command.end
            continue
        }

        if (ch === "^" || ch === "_") {
            var groupStart = skipLatexWhitespace(input, i + 1)
            var group = readLatexGroup(input, groupStart)
            var tag = ch === "^" ? "sup" : "sub"
            html += "<" + tag + ">" + renderLatexTokens(group.text) + "</" + tag + ">"
            i = group.end
            continue
        }

        if (ch === "{") {
            var nested = readLatexGroup(input, i)
            html += renderLatexTokens(nested.text)
            i = nested.end
            continue
        }
        if (ch === "}") {
            ++i
            continue
        }

        html += escapeHtml(ch)
        ++i
    }
    return html
}

function renderInlineMath(value) {
    var formula = renderLatexTokens((value || "").replace(/\n/g, " "))
    return "<span style=\"font-family:'Times New Roman','Cambria Math',serif; color:#42526d; background:#eef4ff; border-radius:4px;\">"
        + formula + "</span>"
}

function renderMathBlock(value) {
    var formula = renderLatexTokens(value || "").replace(/\n/g, "<br/>")
    return "<div style=\"margin:7px 0; padding:8px 10px; text-align:center; background:#eef4ff; border:1px solid #d8e6ff; border-radius:7px;\">"
        + "<span style=\"font-family:'Times New Roman','Cambria Math',serif; color:#314158;\">"
        + formula + "</span></div>"
}

function renderMarkdownRun(value) {
    var html = escapeHtml(value || "")
    html = html.replace(/`([^`]+)`/g, "<span style=\"font-family:monospace; background:#eef2f8; color:#314158;\">$1</span>")
    html = html.replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, function(match, label, href) {
        if (!isAllowedHref(href)) {
            return label
        }
        return "<a href=\"" + escapeAttribute(href) + "\">" + label + "</a>"
    })
    html = html.replace(/\*\*([^*]+)\*\*/g, "<b>$1</b>")
    html = html.replace(/__([^_]+)__/g, "<b>$1</b>")
    html = html.replace(/~~([^~]+)~~/g, "<s>$1</s>")
    html = html.replace(/(^|[\s(])\*([^*\n]+)\*/g, "$1<i>$2</i>")
    html = html.replace(/(^|[\s(])_([^_\n]+)_/g, "$1<i>$2</i>")
    return html
}

function renderInlineMarkdown(value) {
    var input = value || ""
    var result = ""
    var normal = ""
    var i = 0

    function flushNormal() {
        if (normal.length > 0) {
            result += renderMarkdownRun(normal)
            normal = ""
        }
    }

    while (i < input.length) {
        if (input.substr(i, 2) === "\\(") {
            var parenEnd = input.indexOf("\\)", i + 2)
            if (parenEnd >= 0) {
                flushNormal()
                result += renderInlineMath(input.slice(i + 2, parenEnd))
                i = parenEnd + 2
                continue
            }
        }
        if (input.charAt(i) === "$" && input.charAt(i + 1) !== "$") {
            var dollarEnd = findUnescaped(input, "$", i + 1)
            if (dollarEnd >= 0) {
                flushNormal()
                result += renderInlineMath(input.slice(i + 1, dollarEnd))
                i = dollarEnd + 1
                continue
            }
        }
        normal += input.charAt(i)
        ++i
    }
    flushNormal()
    return result
}

function renderParagraph(lines) {
    return "<p style=\"margin:0 0 6px 0;\">" + renderInlineMarkdown(lines.join(" ").replace(/\s+/g, " ")) + "</p>"
}

function renderRichText(value) {
    var input = value || ""
    var lines = input.replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n")
    var html = []
    var paragraph = []
    var listItems = []
    var orderedList = false
    var mathLines = []
    var mathEndMarker = ""

    function flushParagraph() {
        if (paragraph.length > 0) {
            html.push(renderParagraph(paragraph))
            paragraph = []
        }
    }

    function flushList() {
        if (listItems.length === 0) {
            return
        }
        var tag = orderedList ? "ol" : "ul"
        html.push("<" + tag + " style=\"margin:0 0 6px 18px; padding:0;\">" + listItems.join("") + "</" + tag + ">")
        listItems = []
        orderedList = false
    }

    function flushMath() {
        if (mathLines.length > 0) {
            html.push(renderMathBlock(mathLines.join("\n")))
            mathLines = []
        }
        mathEndMarker = ""
    }

    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i]
        var trimmed = line.replace(/^\s+|\s+$/g, "")

        if (mathEndMarker.length > 0) {
            var mathEnd = trimmed.indexOf(mathEndMarker)
            if (mathEnd >= 0) {
                mathLines.push(trimmed.slice(0, mathEnd))
                flushMath()
                var rest = trimmed.slice(mathEnd + mathEndMarker.length).replace(/^\s+/, "")
                if (rest.length > 0) {
                    paragraph.push(rest)
                }
            } else {
                mathLines.push(line)
            }
            continue
        }

        if (trimmed.length === 0) {
            flushParagraph()
            flushList()
            continue
        }

        if (trimmed.indexOf("$$") === 0 || trimmed.indexOf("\\[") === 0) {
            flushParagraph()
            flushList()
            var startMarker = trimmed.indexOf("$$") === 0 ? "$$" : "\\["
            mathEndMarker = startMarker === "$$" ? "$$" : "\\]"
            var body = trimmed.slice(startMarker.length)
            var inlineEnd = body.indexOf(mathEndMarker)
            if (inlineEnd >= 0) {
                mathLines.push(body.slice(0, inlineEnd))
                flushMath()
            } else {
                mathLines.push(body)
            }
            continue
        }

        var heading = trimmed.match(/^(#{1,4})\s+(.+)$/)
        if (heading) {
            flushParagraph()
            flushList()
            var size = Math.max(14, 20 - heading[1].length)
            html.push("<h3 style=\"margin:4px 0 6px 0; font-size:" + size + "px;\">" + renderInlineMarkdown(heading[2]) + "</h3>")
            continue
        }

        var quote = trimmed.match(/^>\s?(.*)$/)
        if (quote) {
            flushParagraph()
            flushList()
            html.push("<blockquote style=\"margin:0 0 6px 0; padding-left:9px; color:#5d6b7f; border-left:3px solid #c5d4ea;\">"
                      + renderInlineMarkdown(quote[1]) + "</blockquote>")
            continue
        }

        var bullet = trimmed.match(/^[-*+]\s+(.+)$/)
        var ordered = trimmed.match(/^\d+[.)]\s+(.+)$/)
        if (bullet || ordered) {
            flushParagraph()
            var nextOrdered = !!ordered
            if (listItems.length > 0 && orderedList !== nextOrdered) {
                flushList()
            }
            orderedList = nextOrdered
            listItems.push("<li>" + renderInlineMarkdown((bullet || ordered)[1]) + "</li>")
            continue
        }

        paragraph.push(line)
    }

    flushParagraph()
    flushList()
    flushMath()

    return "<html><body style=\"margin:0; padding:0; background:transparent;\">" + html.join("") + "</body></html>"
}
