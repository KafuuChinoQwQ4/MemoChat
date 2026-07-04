.pragma library
const clamp01 = (value) => Math.max(0, Math.min(1, value));
const stageValue = (revealProgress, start, span) => {
  if (span <= 0) {
    return revealProgress >= start ? 1 : 0;
  }
  return clamp01((revealProgress - start) / span);
};
const r18NavigationItems = () => [
  { label: "\u4E3B\u9875", icon: "qrc:/icons/r18_home.png", mode: 0 },
  { label: "\u672C\u5730", icon: "qrc:/icons/r18_local.png", mode: 5 },
  { label: "\u5BFC\u822A", icon: "qrc:/icons/r18_navigation.png", mode: 1 },
  { label: "\u6570\u636E\u6E90", icon: "qrc:/icons/r18_datasource.png", mode: 4 }
];
const defaultAgentGameSetupKind = (kind) => kind && kind.length > 0 ? kind : "multi";
