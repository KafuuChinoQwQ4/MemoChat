export function scaledWindowWidth(baseWindowWidth: number, scaleFactor: number): number {
    return Math.round(baseWindowWidth * scaleFactor)
}

export function scaledWindowHeight(baseWindowHeight: number, speechBubbleSafeHeight: number, scaleFactor: number): number {
    return speechBubbleSafeHeight + Math.round(baseWindowHeight * scaleFactor)
}

export function clamp(value: number, minValue: number, maxValue: number): number {
    return Math.max(minValue, Math.min(value, maxValue))
}

export interface ScreenLike {
    desktopAvailableWidth: number
    desktopAvailableHeight: number
    width: number
    height: number
}

export interface Area {
    x: number
    y: number
    width: number
    height: number
}

export function availableArea(screen: ScreenLike): Area {
    return {
        x: 0,
        y: 0,
        width: screen.desktopAvailableWidth > 0 ? screen.desktopAvailableWidth : screen.width,
        height: screen.desktopAvailableHeight > 0 ? screen.desktopAvailableHeight : screen.height,
    }
}

export interface Rect {
    x: number
    y: number
    width: number
    height: number
}

export interface Point {
    x: number
    y: number
}

export function sidePanelPosition(host: Rect, panel: Rect, screen: ScreenLike, gap: number): Point {
    const area = availableArea(screen)
    const rightX = host.x + host.width + gap
    const leftX = host.x - panel.width - gap
    const rightSpace = area.x + area.width - rightX
    const leftSpace = leftX - area.x
    const rightFits = rightSpace >= panel.width
    const leftFits = leftSpace >= 0
    const nextX = rightFits || (!leftFits && rightSpace >= leftSpace) ? rightX : leftX
    const desiredY = host.y + Math.round((host.height - panel.height) / 2)
    return {
        x: clamp(nextX, area.x + 8, area.x + area.width - panel.width - 8),
        y: clamp(desiredY, area.y + 8, area.y + area.height - panel.height - 8),
    }
}

export function languageCodeFromIndex(index: number): string {
    switch (index) {
        case 1:
            return "ja-JP"
        case 2:
            return "en-US"
        case 3:
            return "ko-KR"
        case 4:
            return "fr-FR"
        case 5:
            return "es-ES"
        default:
            return "zh-CN"
    }
}

export function stringValue(value: unknown): string {
    if (value === undefined || value === null) {
        return ""
    }
    return String(value)
}

export interface SpeechSettings {
    speechRules?: unknown
    [key: string]: unknown
}

export function settingsSpeechRulesText(settings: SpeechSettings | null | undefined): string {
    if (!settings || settings.speechRules === undefined) {
        return ""
    }
    return stringValue(settings.speechRules).trim()
}

export function petSettingText(settings: SpeechSettings | null | undefined, name: string): string {
    if (!settings || settings[name] === undefined || settings[name] === null) {
        return ""
    }
    return stringValue(settings[name]).trim()
}

export function settingsVoicePath(voiceDirectory: string, defaultVoice: string): string {
    if (voiceDirectory.length === 0 || defaultVoice.length === 0) {
        return ""
    }
    if (
        defaultVoice.indexOf("/") === 0 ||
        defaultVoice.indexOf("\\") === 0 ||
        defaultVoice.indexOf(":/") > 0 ||
        /^[A-Za-z]:[\\/]/.test(defaultVoice)
    ) {
        return defaultVoice
    }
    const separator = voiceDirectory.endsWith("/") || voiceDirectory.endsWith("\\") ? "" : "/"
    return voiceDirectory + separator + defaultVoice
}

export function settingsVoiceName(defaultVoice: string, fallbackName: string): string {
    if (defaultVoice.length > 0) {
        const normalized = defaultVoice.replace(/\\/g, "/")
        const baseName = normalized.split("/").pop()!
        return baseName.replace(/\.[^.]+$/, "")
    }
    return fallbackName
}

export function providerRuntimeAvailable(
    currentModel: string,
    availableModels: unknown[] | null | undefined,
    providerStatus: string
): boolean {
    if (currentModel.length > 0) {
        return true
    }
    if (availableModels && availableModels.length > 0) {
        return true
    }
    return providerStatus.indexOf("已接入") >= 0
}

export interface QtNamespace {
    Window: number
    Tool: number
    FramelessWindowHint: number
    WindowStaysOnTopHint: number
    WindowTransparentForInput: number
}

export function petWindowFlags(
    qt: QtNamespace,
    isLinux: boolean,
    alwaysOnTop: boolean,
    clickThrough: boolean
): number {
    let nextFlags = (isLinux ? qt.Window : qt.Tool) | qt.FramelessWindowHint
    if (alwaysOnTop) {
        nextFlags |= qt.WindowStaysOnTopHint
    }
    if (clickThrough) {
        nextFlags |= qt.WindowTransparentForInput
    }
    return nextFlags
}
