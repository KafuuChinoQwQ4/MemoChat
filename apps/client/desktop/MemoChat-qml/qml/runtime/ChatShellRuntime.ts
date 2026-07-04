export const clamp01 = (value: number): number =>Math.max(0, Math.min(1, value));

export const stageValue = (revealProgress: number, start: number, span: number): number => {
    if (span <= 0) {
        return revealProgress >= start ? 1 : 0;
    }
    return clamp01((revealProgress - start) / span);
};

interface NavigationItem {
    label: string;
    icon: string;
    mode: number;
}

export const r18NavigationItems = (): NavigationItem[] => [
    { label: "主页", icon: "qrc:/icons/r18_home.png", mode: 0 },
    { label: "本地", icon: "qrc:/icons/r18_local.png", mode: 5 },
    { label: "导航", icon: "qrc:/icons/r18_navigation.png", mode: 1 },
    { label: "数据源", icon: "qrc:/icons/r18_datasource.png", mode: 4 },
];

export const defaultAgentGameSetupKind = (kind: string): string =>
    kind && kind.length > 0 ? kind : "multi";
