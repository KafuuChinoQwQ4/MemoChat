.pragma library
const availableScreenGeometry = (win) => {
  const screenObj = win && win.screen ? win.screen : Qt.application.screens && Qt.application.screens.length > 0 ? Qt.application.screens[0] : null;
  if (!screenObj || !screenObj.availableGeometry) {
    return null;
  }
  const area = screenObj.availableGeometry;
  if (area.x === void 0 || area.y === void 0 || area.width === void 0 || area.height === void 0) {
    return null;
  }
  return area;
};
const centerPointForSize = (area, width, height) => {
  return {
    x: area.x + Math.round((area.width - width) / 2),
    y: area.y + Math.round((area.height - height) / 2)
  };
};
const clampedWindowPosition = (area, width, height) => {
  if (!area) {
    return null;
  }
  const centered = centerPointForSize(area, width, height);
  const maxX = area.x + Math.max(0, area.width - width);
  const maxY = area.y + Math.max(0, area.height - height);
  return {
    x: Math.max(area.x, Math.min(centered.x, maxX)),
    y: Math.max(area.y, Math.min(centered.y, maxY))
  };
};
const shouldHideResize = (win, targetWidth, targetHeight) => {
  if (!win || !win.visible) {
    return false;
  }
  return Math.round(win.width) !== Math.round(targetWidth) || Math.round(win.height) !== Math.round(targetHeight);
};
const targetWindowSize = (chatPageActive, loginSize, chatSize) => {
  return chatPageActive ? chatSize : loginSize;
};
