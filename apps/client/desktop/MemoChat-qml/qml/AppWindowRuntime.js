.pragma library

function availableScreenGeometry(win) {
    const screenObj = win && win.screen ? win.screen
                                         : (Qt.application.screens && Qt.application.screens.length > 0
                                            ? Qt.application.screens[0]
                                            : null)
    if (!screenObj || !screenObj.availableGeometry) {
        return null
    }

    const area = screenObj.availableGeometry
    if (area.x === undefined || area.y === undefined
            || area.width === undefined || area.height === undefined) {
        return null
    }

    return area
}

function centerPointForSize(area, width, height) {
    return {
        "x": area.x + Math.round((area.width - width) / 2),
        "y": area.y + Math.round((area.height - height) / 2)
    }
}

function clampedWindowPosition(area, width, height) {
    if (!area) {
        return null
    }

    const centered = centerPointForSize(area, width, height)
    const maxX = area.x + Math.max(0, area.width - width)
    const maxY = area.y + Math.max(0, area.height - height)
    return {
        "x": Math.max(area.x, Math.min(centered.x, maxX)),
        "y": Math.max(area.y, Math.min(centered.y, maxY))
    }
}

function shouldHideResize(win, targetWidth, targetHeight) {
    if (!win || !win.visible) {
        return false
    }

    return Math.round(win.width) !== Math.round(targetWidth)
            || Math.round(win.height) !== Math.round(targetHeight)
}

function targetWindowSize(chatPageActive, loginSize, chatSize) {
    return chatPageActive ? chatSize : loginSize
}
