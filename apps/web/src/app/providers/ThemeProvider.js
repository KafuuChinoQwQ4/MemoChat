import { Fragment as _Fragment, jsx as _jsx } from "react/jsx-runtime";
import { useEffect } from "react";
import { useSettingsStore } from "@/features/settings/store/settingsStore";
export function ThemeProvider({ children }) {
    const theme = useSettingsStore((s) => s.theme);
    const blurEnabled = useSettingsStore((s) => s.blurEnabled);
    useEffect(() => {
        document.documentElement.setAttribute("data-theme", theme);
    }, [theme]);
    useEffect(() => {
        if (!blurEnabled) {
            document.documentElement.classList.add("no-blur");
        }
        else {
            document.documentElement.classList.remove("no-blur");
        }
    }, [blurEnabled]);
    return _jsx(_Fragment, { children: children });
}
