pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15

Column {
    id: root

    property string text: ""
    property color textColor: "#253247"
    property color codeTextColor: "#314158"
    property int textPixelSize: 14
    property int codePixelSize: 12
    property real maxCodeBlockHeight: 520
    property var keywordCache: ({})
    readonly property var segments: parseSegments(text)

    spacing: 7

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

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

    function keywordMap(lang) {
        var key = normalizeLanguage(lang)
        if (keywordCache[key]) {
            return keywordCache[key]
        }
        var map = {}
        var words = keywordSource(key).split(/\s+/)
        for (var i = 0; i < words.length; ++i) {
            if (words[i].length > 0) {
                map[words[i].toLowerCase()] = true
            }
        }
        keywordCache[key] = map
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

    function highlightCodeRun(value, lang) {
        var map = keywordMap(lang)
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

    function highlightLine(line, lang, state) {
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
            result += highlightCodeRun(line.slice(start, i), lang)
        }

        return result
    }

    function highlightedCode(value, language) {
        var lang = normalizeLanguage(language)
        var state = { "blockEnd": "" }
        var lines = (value || "").replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n")
        var html = []
        for (var i = 0; i < lines.length; ++i) {
            html.push(highlightLine(lines[i], lang, state))
        }
        return "<html><body style=\"margin:0; padding:0; background:transparent; color:" + codeTextColor + ";\">" + html.join("<br/>") + "</body></html>"
    }

    Repeater {
        model: root.segments

        delegate: Item {
            id: segmentDelegate
            required property var modelData

            width: Math.max(80, root.width)
            height: modelData.kind === "code" ? codeFrame.implicitHeight : paragraphText.implicitHeight

            TextEdit {
                id: paragraphText
                visible: segmentDelegate.modelData.kind !== "code"
                width: parent.width
                text: segmentDelegate.modelData.text || ""
                color: root.textColor
                font.pixelSize: root.textPixelSize
                wrapMode: TextEdit.Wrap
                textFormat: Text.PlainText
                readOnly: true
                selectByMouse: true
                cursorVisible: false
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0
            }

            Rectangle {
                id: codeFrame
                visible: segmentDelegate.modelData.kind === "code"
                width: parent.width
                implicitHeight: codeHeader.height + codeBody.height
                radius: 9
                color: Qt.rgba(0.90, 0.95, 1.0, 0.72)
                border.width: 1
                border.color: Qt.rgba(0.35, 0.61, 0.90, 0.26)
                clip: true

                Rectangle {
                    id: codeHeader
                    width: parent.width
                    height: 30
                    color: Qt.rgba(0.82, 0.90, 1.0, 0.54)

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.languageLabel(segmentDelegate.modelData.language || "")
                        color: "#55708f"
                        font.pixelSize: 11
                        font.bold: true
                        elide: Text.ElideRight
                        width: parent.width - 20
                    }
                }

                Rectangle {
                    id: codeBody
                    anchors.top: codeHeader.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: root.clamp(codeFlick.contentHeight + 14, 42, root.maxCodeBlockHeight)
                    color: Qt.rgba(0.97, 0.99, 1.0, 0.66)

                    Flickable {
                        id: codeFlick
                        anchors.fill: parent
                        anchors.margins: 7
                        clip: true
                        contentWidth: Math.max(width, codeText.implicitWidth + 2)
                        contentHeight: codeText.implicitHeight + 2
                        boundsBehavior: Flickable.StopAtBounds
                        flickableDirection: Flickable.HorizontalAndVerticalFlick
                        interactive: contentWidth > width || contentHeight > height

                        TextEdit {
                            id: codeText
                            x: 0
                            y: 0
                            width: Math.max(codeFlick.width, implicitWidth)
                            text: root.highlightedCode(segmentDelegate.modelData.text || "", segmentDelegate.modelData.language || "")
                            color: root.codeTextColor
                            font.family: "Consolas"
                            font.pixelSize: root.codePixelSize
                            textFormat: TextEdit.RichText
                            wrapMode: TextEdit.NoWrap
                            readOnly: true
                            selectByMouse: true
                            cursorVisible: false
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                        }

                        ScrollBar.horizontal: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            height: 6
                            contentItem: Rectangle {
                                implicitHeight: 4
                                radius: 2
                                color: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                            }
                            background: Rectangle {
                                implicitHeight: 6
                                radius: 3
                                color: Qt.rgba(0.35, 0.61, 0.90, 0.10)
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            width: 6
                            contentItem: Rectangle {
                                implicitWidth: 4
                                radius: 2
                                color: Qt.rgba(0.35, 0.61, 0.90, 0.38)
                            }
                            background: Rectangle {
                                implicitWidth: 6
                                radius: 3
                                color: Qt.rgba(0.35, 0.61, 0.90, 0.10)
                            }
                        }
                    }
                }
            }
        }
    }
}
