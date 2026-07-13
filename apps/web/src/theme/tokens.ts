/** CSS variable names for the MemoChat Liquid Glass design system.
 *  Use these constants when reading/writing CSS vars via JavaScript.
 */
export const TOKEN = {
  glassBlur: "--glass-blur",
  glassRadius: "--glass-radius",
  glassFill: "--glass-fill",
  glassStroke: "--glass-stroke",
  glassSheenHeight: "--glass-sheen-height",
  glassBtnText: "--glass-btn-text",
  glassBtnBg: "--glass-btn-bg",
  glassBtnBgHover: "--glass-btn-bg-hover",
  glassBtnBgPress: "--glass-btn-bg-press",
  glassInputBorder: "--glass-input-border",
  glassInputBorderFocus: "--glass-input-border-focus",
  panelRadius: "--panel-radius",
  tintHover: "--tint-hover",
  tintPress: "--tint-press",
  tintSelected: "--tint-selected",
  colorBadge: "--color-badge",
  colorBrandGreen: "--color-brand-green",
  colorBrandGreenDark: "--color-brand-green-dark",
  colorBrand: "--color-brand",
  colorBrandDark: "--color-brand-dark",
  sidebarW: "--sidebar-w",
  listPanelW: "--list-panel-w",
  colGap: "--col-gap",
  fontFamilyZh: "--font-family-zh",
  glassBlurEnabled: "--glass-blur-enabled",
} as const

export type ThemeToken = (typeof TOKEN)[keyof typeof TOKEN]
