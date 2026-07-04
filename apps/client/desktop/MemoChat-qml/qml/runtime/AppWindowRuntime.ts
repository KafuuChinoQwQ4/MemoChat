declare const Qt: {
  application: {
    screens?: Screen[]
  }
}

interface Rect {
  x: number
  y: number
  width: number
  height: number
}

interface Screen {
  availableGeometry?: Rect
}

interface Window {
  screen?: Screen
  visible?: boolean
  width?: number
  height?: number
}

interface Point {
  x: number
  y: number
}

interface Size {
  width: number
  height: number
}

export const availableScreenGeometry = (win: Window | null | undefined): Rect | null => {
  const screenObj: Screen | null =
    win && win.screen
      ? win.screen
      : Qt.application.screens && Qt.application.screens.length > 0
        ? Qt.application.screens[0]
        : null

  if (!screenObj || !screenObj.availableGeometry) {
    return null
  }

  const area = screenObj.availableGeometry
  if (
    area.x === undefined ||
    area.y === undefined ||
    area.width === undefined ||
    area.height === undefined
  ) {
    return null
  }

  return area
}

export const centerPointForSize = (area: Rect, width: number, height: number): Point => {
  return {
    x: area.x + Math.round((area.width - width) / 2),
    y: area.y + Math.round((area.height - height) / 2),
  }
}

export const clampedWindowPosition = (
  area: Rect | null | undefined,
  width: number,
  height: number
): Point | null => {
  if (!area) {
    return null
  }

  const centered = centerPointForSize(area, width, height)
  const maxX = area.x + Math.max(0, area.width - width)
  const maxY = area.y + Math.max(0, area.height - height)
  return {
    x: Math.max(area.x, Math.min(centered.x, maxX)),
    y: Math.max(area.y, Math.min(centered.y, maxY)),
  }
}

export const shouldHideResize = (
  win: Window | null | undefined,
  targetWidth: number,
  targetHeight: number
): boolean => {
  if (!win || !win.visible) {
    return false
  }

  return (
    Math.round(win.width!) !== Math.round(targetWidth) ||
    Math.round(win.height!) !== Math.round(targetHeight)
  )
}

export const targetWindowSize = (
  chatPageActive: boolean,
  loginSize: Size,
  chatSize: Size
): Size => {
  return chatPageActive ? chatSize : loginSize
}
