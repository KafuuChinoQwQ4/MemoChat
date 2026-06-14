.pragma library

function scaledWindowWidth(baseWindowWidth, scaleFactor) {
    return Math.round(baseWindowWidth * scaleFactor)
}

function scaledWindowHeight(baseWindowHeight, speechBubbleSafeHeight, scaleFactor) {
    return speechBubbleSafeHeight + Math.round(baseWindowHeight * scaleFactor)
}

function clamp(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(value, maxValue))
}

function availableArea(screen) {
    return {
        "x": 0,
        "y": 0,
        "width": screen.desktopAvailableWidth > 0 ? screen.desktopAvailableWidth : screen.width,
        "height": screen.desktopAvailableHeight > 0 ? screen.desktopAvailableHeight : screen.height
    }
}

function sidePanelPosition(host, panel, screen, gap) {
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
        "x": clamp(nextX, area.x + 8, area.x + area.width - panel.width - 8),
        "y": clamp(desiredY, area.y + 8, area.y + area.height - panel.height - 8)
    }
}

function languageCodeFromIndex(index) {
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

function stringValue(value) {
    if (value === undefined || value === null) {
        return ""
    }
    return String(value)
}

function settingsSpeechRulesText(settings) {
    if (!settings || settings.speechRules === undefined) {
        return ""
    }
    return stringValue(settings.speechRules).trim()
}

function petSettingText(settings, name) {
    if (!settings || settings[name] === undefined || settings[name] === null) {
        return ""
    }
    return stringValue(settings[name]).trim()
}

function settingsVoicePath(voiceDirectory, defaultVoice) {
    if (voiceDirectory.length === 0 || defaultVoice.length === 0) {
        return ""
    }
    if (defaultVoice.indexOf("/") === 0 || defaultVoice.indexOf("\\") === 0
            || defaultVoice.indexOf(":/") > 0 || /^[A-Za-z]:[\\/]/.test(defaultVoice)) {
        return defaultVoice
    }
    const separator = voiceDirectory.endsWith("/") || voiceDirectory.endsWith("\\") ? "" : "/"
    return voiceDirectory + separator + defaultVoice
}

function settingsVoiceName(defaultVoice, fallbackName) {
    if (defaultVoice.length > 0) {
        const normalized = defaultVoice.replace(/\\/g, "/")
        const baseName = normalized.split("/").pop()
        return baseName.replace(/\.[^.]+$/, "")
    }
    return fallbackName
}

function providerRuntimeAvailable(currentModel, availableModels, providerStatus) {
    if (currentModel.length > 0) {
        return true
    }
    if (availableModels && availableModels.length > 0) {
        return true
    }
    return providerStatus.indexOf("已接入") >= 0
}

function petWindowFlags(qt, isLinux, alwaysOnTop, clickThrough) {
    var nextFlags = (isLinux ? qt.Window : qt.Tool) | qt.FramelessWindowHint
    if (alwaysOnTop) {
        nextFlags |= qt.WindowStaysOnTopHint
    }
    if (clickThrough) {
        nextFlags |= qt.WindowTransparentForInput
    }
    return nextFlags
}
