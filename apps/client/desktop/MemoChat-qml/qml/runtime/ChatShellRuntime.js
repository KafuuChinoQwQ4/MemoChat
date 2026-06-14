.pragma library

function clamp01(value) {
    return Math.max(0, Math.min(1, value))
}

function stageValue(revealProgress, start, span) {
    if (span <= 0) {
        return revealProgress >= start ? 1 : 0
    }
    return clamp01((revealProgress - start) / span)
}

function r18NavigationItems() {
    return [
        { "label": "主页", "icon": "qrc:/icons/r18_home.png", "mode": 0 },
        { "label": "本地", "icon": "qrc:/icons/r18_local.png", "mode": 5 },
        { "label": "导航", "icon": "qrc:/icons/r18_navigation.png", "mode": 1 },
        { "label": "数据源", "icon": "qrc:/icons/r18_datasource.png", "mode": 4 }
    ]
}

function defaultAgentGameSetupKind(kind) {
    return kind && kind.length > 0 ? kind : "multi"
}
